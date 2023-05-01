Telescope Simulated Annealer
============================

These files are meant only to demonstrate my personal coding
capabilities and style.  They are not intended for any other purpose
or use.

Overview.
--------------------------------------------------

**Generic Problem.**
This code solves a kind of double-option variation of the classical
traveling sales problem.  In this variation, no "destination" is fixed
because each is actually accessible from two coordinate
configurations.  The problem is therefore not merely to order the
destinations well, but also to optimize the choice of which
configurations to use.

**The Specific Problem.**
This problem became of interest while using a small backyard
telescope.  Telescopes turn horizontally side to side as well as
vertically up and down, but some may be able to move vertically
*beyond straight upwards*.  If so, every direction in the sky can be
viewed using two telescope configurations, not just one.

**The Code.**
The entire program is run by the shell script `run.sh`, *using input
specified in `run_details.csv`.  There are essentially *three parts to
the program:

1. Python scripts that generate instances of the problem at arbitrary
size (see `pyth/setup.py`) from what is specified in
`run_details.csv`. These initial processing and randomized setup steps
use a bit of [pandas](https://pandas.pydata.org/) and
[NumPy](https://numpy.org/), though not much.  The script
`pyth/prepare_details.py` then prepares all of these problem instances
to be solved using a temporary `temp.sh` file that runs the remainder
of the program.

2. C++ code to solve the problem two ways:

   (a) using a greedy algorithm (`src/TelGreedy.h`), and

   (b) using a simulated annealing algorithm (in `src/TelAnnealer.h`,
   which derives from a virtual class useful for a generic simulated
   annealing problem, in `src/SimAnneal.h`).

   The annealing algorithm uses randomized reorderings and is very
   closely based on the traveling sales solution presented in the book
   *Finite Markov Chains and Algorithmic Applications* by O.
   Häggström.  The permutations to randomly reorder here are the same
   as used there, but the distinction that here we also allow swaps of
   the configurations.  Annealing controls are also specified within
   `run_details.csv`, which do need manual tuning for each problem.

3. Python scripts using matplotlib to plot the telescope schedules
(`pyth/plot.py` and `pyth/animate.py`) and plot summary graphs
(`pyth/summary.py`).

Some sample output is shown below.

| Greedy Solution | Annealing Solution |
| --------------- | ------------------ |
| ![Greedy solution](./output/run-20/greedy-schedule.png) | ![Annealing solution](./output/run-20/simanneal-best-10000000.png)

| Greedy Solution | Annealing Solution |
| --------------- | ------------------ |
| ![Greedy solution](./output/run-40/greedy-schedule.png) | ![Annealing solution](./output/run-40/simanneal-best-15000000.png) |


<img
    style="width:75%; max-width:600px"
    alt="Annealing solution"
    src="./output/run-40/simanneal-animation-faster.png"
/>

Interpret these pictures almost as if looking into the sky.  Each dot
is a desired direction.  The color—orange or blue—indicates which
telescope orientation to use.  (The sole green dot is the starting
location.)

**Distances.** The polar angle position represents horizontal
*telescope rotations and the radial distance represents a spherical
*angle between the telescope and the vertical, straight-up direction.
*(In mathematics, such spherical coordinates are traditionally denoted
*by $\theta$ and $\phi$, respectively.)  Blue points have a positive
*spherical angle, while orange points have a negative spherical angle.
*The formula used to determine distance between two configurations
*$(\theta_1, \phi_1)$ and $(\theta_2, \phi_2)$ is

$$ \max\left\{
    |\phi_1 - \phi_2|,
    \min\begin{pmatrix}
            |\theta_1 - \theta_2|, \\
            |\theta_1 - \theta_2 - 2\pi|, \\
            |\theta_1 - \theta_2 + 2\pi|
    \end{pmatrix}
    \right\} $$

The triple-minimum assumes the telescope can rotate horizontally
beyond $2\pi$ without issue.  The maximum models a telescope that has
two separate hinges for $\theta$ and $\phi$ and can adjust each of
them independently at the same speed.

References.
--------------------------------------------------

- Olle Häggström,
  *Finite Markov Chains and Algorithmic Applications*,
  Cambridge University Press, Cambridge, 2002.

+ J. D. Hunter,
  *Matplotlib: A 2D graphics environment*,
  Computing in Science & Engineering **9** (2007), no. 3, 90–95.
