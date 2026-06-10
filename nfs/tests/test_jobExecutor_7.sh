killall progDelay
./bin/jobCommander localhost 7856 issueJob ./progDelay 1000
./bin/jobCommander localhost 7856 stop job_2
./bin/jobCommander localhost 7856 exit
