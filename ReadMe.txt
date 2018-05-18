
This project simulates a collection of balls moving in 2D within a closed container,
colliding elastically with each other and with the container walls. It started as a
projected written by my son Adam in JavaScript, using a fixed time delta approach to
model successive states of the system. But this approach falls down when there are
multiple collisions within a given step. This new versions uses a computationally
intensive but more accurate method, finding the time to the next collision and then
running the simulation to that precise point. There are various optimizations to take
advantage of multiple processors, and to cull collisions that obviously can't occur.

Have fun!
