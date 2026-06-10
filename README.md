# Systems Programming Projects (Course K24)

[cite_start]Καλωσήρθατε στο αποθετήριο για το μάθημα **Προγραμματισμός Συστήματος (Κ24)** του Τμήματος Πληροφορικής και Τηλεπικοινωνιών (DIT UoA). 

[cite_start]Αυτό το repository περιλαμβάνει δύο ολοκληρωμένες εργασίες που εστιάζουν στη διαχείριση διεργασιών, τον παράλληλο προγραμματισμό, την επικοινωνία IPC, τον συγχρονισμό νημάτων (threads) και τη δικτύωση σε περιβάλλον Unix/Linux.

---

## Δομή του Repository

Το project είναι χωρισμένο σε δύο ανεξάρτητα μέρη:

### [Part 1: FileSync System (FSS)](./ffs)
Μια low-level εφαρμογή συγχρονισμού αρχείων σε πραγματικό χρόνο μεταξύ τοπικών καταλόγων.
**Βασικές Τεχνολογίες:** `fork()`, `exec*()`, `inotify` API, Named Pipes (FIFOs), Low-level I/O syscalls.
**Συνιστώσες:** `fss_manager`, `fss_console`, και ένα βοηθητικό Bash script.

### [Part 2: Network Job Executor (NFS-Style)](./nfs)
Ένα κατανεμημένο σύστημα εκτέλεσης διεργασιών (jobs) μέσω δικτύου με αρχιτεκτονική Client-Server.
* **Βασικές Τεχνολογίες:** TCP Sockets, POSIX Threads (Thread Pool), Mutexes, Condition Variables.
* **Συνιστώσες:** `jobCommander` (Client), `jobExecutorServer` (Server), και αυτοματοποιημένα testing scripts.

---

## 🛠️ Γενικές Οδηγίες Μεταγλώττισης
Κάθε φάκελος περιέχει το δικό του αυτόνομο `Makefile` για separate compilation[cite: 299]. Για να κάνετε compile τα εκτελέσιμα, πλοηγηθείτε στον αντίστοιχο φάκελο και τρέξτε:

```bash
make
