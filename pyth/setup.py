from math import pi, sin
import os
import sys

import numpy as np

########################################

class Direction:
    def __init__(self, id, theta, phi):
        self.id = id
        self.theta = theta
        self.phi = phi
        self.other = [theta + pi * (theta < pi) - pi * (theta >= pi), -phi]

    def switch_rep(self):
        self.theta, self.phi, self.other \
            = self.other[0], self.other[1], [self.theta, self.phi]

    def dist_to(self, other_dir):
        tdiff = self.theta - other_dir.theta
        return max(
            abs(self.phi - other_dir.phi),
            min(
                abs(tdiff),
                abs(tdiff - 2*pi),
                abs(tdiff + 2*pi)
                )
        )

    def is_other_rep(self):
        return self.phi < 0

    def radial_plot_coords(self):
        t, p = [self.theta, self.phi] if self.phi >= 0 else self.other
        return [t, sin(p)]

    def __str__(self):
        return ( 'Direction(id=%d, theta=%.10f, phi=%.10f)'
                    % (self.id, self.theta, self.phi) )

    def __repr__(self):
        return str(self)

########################################

def total_distance_dirs(dir_schedule):
    """
    Given a schedule as a list of Direction classes specifying successive
    telescope positions, compute the total distance that the telescope moves.
    """

    one, two = iter(dir_schedule), iter(dir_schedule)
    next(two) # drop the first location...
    return sum(a.dist_to(b) for a,b in zip(one, two))

########################################

def simulate_locs(run_id, num_locs, save_file, random_seed=1):
    """
    Randomly generate num_locs locations in a 2D array of the form:

        theta_0, phi_0
        theta_1, phi_1
        theta_2, phi_2
        ... etc ...

    By default, phi_2 is always positive, between 0 and 0.98 * pi/2.
    The positions are generated to be uniform over this region.
    """

    np.random.seed(random_seed)
    xyzs = np.zeros(shape=(num_locs, 3))
    to_redraw = np.ones(num_locs, dtype=bool)
    min_z_coord_allowed = np.sin(0.02 * pi/2.)
    while (num_to_redraw := to_redraw.sum()) > 0:
        xyzs[to_redraw, :] = np.random.normal(size=(num_to_redraw, 3))
        xyzs[to_redraw, 2] = np.abs(xyzs[to_redraw, 2])
        # Redraw any where the z coord is still too low:
        to_redraw = to_redraw * (xyzs[:,2] <= min_z_coord_allowed)
    xyzs = np.divide(xyzs.T, np.sqrt(np.sum(xyzs**2, axis=1))).T
    theta = np.arctan2(xyzs[:,0], xyzs[:,1])
    theta = theta + (2*pi)*(theta < 0)
    phi   = np.arcsin(xyzs[:,2])

    if save_file:
        with open(save_file, 'w') as of:
            of.write('Run id: %d\n' % run_id)
            of.write('Num directions: %d\n' % num_locs)
            for i in range(num_locs):
                of.write('Direction(id=%d, theta=%.10f, phi=%.10f)\n'
                            % (i, theta[i], phi[i]))
            of.close()
    else:
        return [Direction(i, t, p) for (i,t,p) in zip(range(num_locs),
                                                        theta, phi) ]

########################################

if __name__ == '__main__':
    if len(sys.argv) <= 1:
        exit()
    run_id = int(sys.argv[1])
    num_locs = int(sys.argv[2])

    input_folder = '../input/'
    os.makedirs(input_folder, exist_ok=True)

    filename = 'directions-%d.txt' % run_id
    simulate_locs(run_id, num_locs, save_file=input_folder + filename, random_seed=run_id)
