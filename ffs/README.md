# Part 1: FileSync System (FSS)

##  Περιγραφή
Το **FileSync System (FSS)** είναι ένα σύστημα αυτοματοποιημένου συγχρονισμού αρχείων σε πραγματικό χρόνο μεταξύ ενός καταλόγου πηγής (`source_dir`) και ενός καταλόγου προορισμού (`target_dir`). Η υλοποίηση βασίζεται αποκλειστικά σε low-level κλήσεις συστήματος του Linux, αποφεύγοντας έτοιμα εργαλεία (όπως `cp` ή `rsync`).

---

## Αρχιτεκτονική & Υλοποίηση

Το σύστημα αποτελείται από τρία κύρια μέρη:

1. **`fss_manager` (Διαχειριστής):**
   * Αρχικοποιεί το σύστημα και διαβάζει τα αρχικά ζεύγη καταλόγων από ένα `config_file`.
   * Χρησιμοποιεί το **`inotify` API** (`inotify_init`, `inotify_add_watch`) για να παρακολουθεί real-time αλλαγές (δημιουργία, τροποποίηση, διαγραφή αρχείων).
   * Διαχειρίζεται ένα όριο ταυτόχρονων εργαζομένων (`worker_limit`) μέσω εσωτερικής ουράς (queue).
   * Ακούει για σήματα `SIGCHLD` για την ομαλή εκκαθάριση των workers μέσω `waitpid()`.

2. **Worker Processes (Εργαζόμενοι):**
   * Δημιουργούνται δυναμικά μέσω `fork()` και `exec()` από τον manager για να εκτελέσουν μια εργασία συγχρονισμού (`FULL`, `ADDED`, `MODIFIED`, `DELETED`).
   * Πραγματοποιούν το I/O χρησιμοποιώντας syscalls όπως `open()`, `read()`, `write()`, `unlink()`.
   * Στέλνουν μια τελική αναφορά (`exec_report`) πίσω στον manager μέσω ανώνυμου pipe.

3. **`fss_console` (Διεπαφή Χρήστη):**
   * Μια CLI κονσόλα που επιτρέπει στον χρήστη να στέλνει δυναμικά εντολές (`add`, `cancel`, `status`, `sync`, `shutdown`) στον manager.
   * Η επικοινωνία μεταξύ manager και κονσόλας γίνεται αμφίδρομα μέσω **Named Pipes (FIFOs)** (`fss_in`, `fss_out`).

4. **`fss_script.sh` (Bash Utility):**
   * Ένα σενάριο κελύφους για τη δημιουργία αναφορών από τα logfiles (`listAll`, `listMonitored`, `listStopped`) και τον καθαρισμό των target καταλόγων (`purge`).

---

## Οδηγίες Εκτέλεσης

### Μεταγλώττιση:
```bash
make
```

# Εκκίνηση Manager:
```bash
./fss_manager -l <manager_logfile> -c <config_file> -n <worker_limit>
```

# Εκκίνηση Κονσόλας:
```bash
./fss_console -l <console_logfile>
```


## Η εργασια αποτελείται από 4 .c αρχεία που βρισκονται στον φακελο src:

# fss_manager.c, fss_console.c, worker.c, sync_info_mem_store (sync_info_store.h και sync_info_mem_store.c)

# fss_manager: 
O fss_manager διαβάζει το config file που περιέχει τα ζεύγη καταλόγων. Για κάθε ζεύγος δημιουργείται μια εγγραφή στη δομή sync_info χρησιμοποιώντας μία linked list.
Δημιουργεί δύο named pipes, το fss_in και fss_out, ώστε να έχει επικοινωνία με το fss_console. Μέσω του fss_in λαμβάνει εντολές από τον χρήστη και μέσω του fss_out στέλνει μηνύματα. Ξεκινά το inotify με inotify_init() και προσθέτει watch για κάθε source directory χρησιμοποιώντας inotify_add_watch(). Με αυτόν τον τρόπο παρακολουθεί αλλαγές όπως δημιουργία, τροποποίηση και διαγραφή αρχείων. Όταν ένα inotify event προκύπτει ο manager διαβάζει το event και περνει πληροφορίες και αν χρειάζεται δημιουργεί ένα task για την ουρα αναμονης (με τα πεδία command, source, target, filename και operation) είτε ξεκινάει άμεσα με call_worker.
Δημιουργεί workers processes χρησιμοποιώντας fork() και execl(). Πριν το fork δημιουργεί ένα pipe ώστε να ανακατευθύνει το stdout από τον worker στο pipe. Έτσι ο manager λαμβάνει το exec report που παράγεται από τον worker και το επεξεργάζεται στην συνάρτηση process_report. Επιπλέον διατηρεί μια μεταβλητή active_workers που αν υπερβεί το worker_limit τα νέα tasks μπαινουν στην ουρά. Μέσα στο κύριο while loop ο manager χρησιμοποιεί select() για να παρακολουθεί τόσο το fss_in όσο και το inotify για events. Ανάλογα με την εντολή (add, status, sync, cancel, shutdown) εκτελούνται οι αντίστοιχες ενέργειες. Ο manager χρησιμοποιει signal handler για το SIGCHLD ο οποίος καλεί waitpid() με επιλογή WNOHANG για να συλλέξει τους worrkers που τελειωνουν. Με την έξοδο ενός worker μειώνει την active_workers και ενημερώνει το sync_info (με την τελευταία ώρα συγχρονισμού και το αποτέλεσμα της εργασίας).

# fss_console.c
Ο χρήστης εισάγει εντολές μέσω του stdin του console. Το πρόγραμμα ανοίγει τα named pipes fss_in για αποστολή εντολων και fss_out για λήψη μηνυμάτων από τον manager.
Χρησιμοποιώ select() ώστε να παρακολουθείται την εισοδο και το fss_out.

# worker.c:
Περνει τέσσερα arguments: source_directory, target_directory, filename και operation.
Αν το operation είναι "FULL" και το filename είναι "ALL"  ο worker ανοίγει τον source directory με opendir() διαβάζει όλα τα regular αρχεία με readdir() και προσπαθεί να τα αντιγράψει στον target χρησιμοποιώντας τις κλήσεις open(), read(), write() και close(). Σε περιπτώσεις "ADDED" και "MODIFIED" αντιγράφει το συγκεκριμένο αρχείο και στην περίπτωση "DELETED" προσπαθεί να διαγράψει το αρχείο στον target με unlink(). Ο worker ελέγχει κάθε κλήσηγια αποτυχία. Σε περίπτωση σφάλματος, χρησιμοποιεί το  errno για να καταγράψει το συγκεκριμένο error στο error_buffer.

# sync_info_mem_store:
Αποτελεί τον κωδικα για την αποθήκευση και αναζήτηση πληροφοριών για τους καταλόγους.
Χρησιμοποιώ μία linked list. Κάθε node δομη LogInfo περιέχει τις μεταβλητές:
source_dir
target_dir
status ("Active", "Running", "Stopped")
last_sync_time
error_count
wd

και υπαρχει μια απλη υλοποιηση ουρας που αποθηκεθει δομες tasks με πεδια:
command 
source 
target 
filename
operation
