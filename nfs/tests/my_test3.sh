./bin/jobCommander localhost 7856 setConcurrency 2
./bin/jobCommander localhost 7856 issueJob ./progDelay 1
./bin/jobCommander localhost 7856 poll
./bin/jobCommander localhost 7856 stop job_2
./bin/jobCommander localhost 7856 issueJob ./progDelay 1
./bin/jobCommander localhost 7856 poll
./bin/jobCommander localhost 7856 issueJob ./progDelay 1
./bin/jobCommander localhost 7856 issueJob ./progDelay 1
./bin/jobCommander localhost 7856 poll
./bin/jobCommander localhost 7856 stop job_4
./bin/jobCommander localhost 7856 poll
./bin/jobCommander localhost 7856 issueJob ./progDelay 1
./bin/jobCommander localhost 7856 issueJob ./progDelay 1
./bin/jobCommander localhost 7856 setConcurrency 3
./bin/jobCommander localhost 7856 stop job_7
./bin/jobCommander localhost 7856 poll
./bin/jobCommander localhost 7856 issueJob ./progDelay 1
./bin/jobCommander localhost 7856 poll
./bin/jobCommander localhost 7856 stop job_7
./bin/jobCommander localhost 7856 poll
./bin/jobCommander localhost 7856 exit