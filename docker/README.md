# gazebo-docker
Docker for development with Gazebo and YARP

Builds on the YARP Docker available at this [link](https://hub.docker.com/repository/docker/eventdrivenrobotics/yarp).

The code directory can be found in the container at `/usr/local/src`. Here you can find all the repositories which are already cloned with specific versions, known to be compatible with each other, built in the `build` directory available inside each repo, and installed system-wise.

The repositories being cloned are: 
* https://github.com/event-driven-robotics/gazebo-yarp-plugins.git
* https://github.com/robotology/icub-gazebo.git
* https://github.com/robotology/icub-main.git
* https://github.com/xEnVrE/vision-based-manipulation-simulation.git
* https://github.com/robotology/yarp.git
* https://github.com/robotology/ycm.git

More specifically, in `/usr/local/share/gazebo/worlds` a set of available gazebo scenarios is provided. It is recommended to run this container with X server forwarding and Nvidia/OpenGL support (check for tags ending with `_opengl`), because the simulator requires graphical rendering which may be heavy to run on CPU.
Please check the helper functions available at https://github.com/event-driven-robotics/docker-resources.git

To run gazebo just type:
`gazebo <PATH_TO_WORLD>`

This will automatically load the YARP plugins compiled from `/usr/local/src/gazebo-yarp-plugins`.
