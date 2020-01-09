# thready - A lightweight and fast scheduling simulator

Thready (thread + ready) simulates a uniprocessor scheduling a sporadic task system according to the earliest deadline first (EDF) algorithm.


## Install

To build the simulator run `make`.
The resulting executable is stand alone,
for convenience you can put in in a folder that is in your `$PATH`.
For example, `make install` copies the executable to `${HOME}/.local/bin`.


## Usage

Print help:
```
$ ./thready -h
Usage: thready [-h] [-v] [-r <statedump.json>] [-z jobtracerandomseed] -n dumpprefix -t breaktime -j <tasksystemfile.json>
```

Simulate an example task system for 10 hours in millisecond resolution:
```
$ ./thready -n my-first-simulation -j test/p41-ts-nointerarrival-nohi.json -t 36000000
36000000: End of simulation with 21697 events servicing 10847 jobs
```

Run examples with different random seeds:
```
$ ./thready -n my-simulation-with-seed-0 -j test/p41-ts-nointerarrival-0.5hi.json -t 36000000 -z 0
Overflowing job of task 1 arrives at 0 with deadline at 20 and computation of 4 which is an overrun of 3 
36000000: End of simulation with 21699 events servicing 10847 jobs
$ ./thready -n my-simulation-with-seed-12 -j test/p41-ts-nointerarrival-0.5hi.json -t 36000000 -z 12
Overflowing job of task 2 arrives at 22677 with deadline at 22697 and computation of 4 which is an overrun of 2 
36000000: End of simulation with 21930 events servicing 10963 jobs
```


## Contributing

Please contact the authors first if you plan to contribute.


## License

This program uses code from:

- the `rnd.h` single header file library for random number generation by Mattias Gustavsson which is in the public domain; and
- `parg.{c,h}` command line argument handling code by Jørgen Ibsen licensed under Creative Commons Zero v1.0; and
- `pqueue.{c,h}` priority queue code by Volkan Yazıcı licensed under BSD 2-Clause "Simplified" License; and
- `json.{c,h}` JSON parsing and printing from libjson by Vincent Hanquez licensed under GNU Lesser General Public License; and
- `selist.{c,h}` space efficient list code by Enno Rehling licensed under the ISC licence.

For license details refer to the mentioned files.
Everything else is licensed under the Creative Commons Zero v1.0 Universal license,
for details refer to the file [`LICENSE`](LICENSE).
