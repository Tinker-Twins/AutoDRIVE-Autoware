# Installation Documentation

> [!NOTE]
> - Installation was tested with a workstation running Ubuntu 22.04, NVIDIA Driver 575.64.03, Docker Engine 28.3.2, and NVIDIA Container Toolkit 1.17.8.
> - A dedicated `Autoware_WS` was created on host workstation to organize `autoware`, `autoware_data`, and `autoware_map`.
> - Pay close attention when executing commands from host workstation versus from within the Docker container.

## Prerequisites:

1. [Docker](https://docs.docker.com/engine) (Required)

2. [NVIDIA CUDA 12 Compatible GPU Driver](https://www.nvidia.com/en-us/drivers)  (Preferred)

3. [NVIDIA Container Toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest) (Preferred)

## Installation:

1. Create a dedicated directory (recommended) and organize the workspace:

    ```bash
    mkdir -p ~/Autoware_WS # Autoware workspace
    cd ~/Autoware_WS
    mkdir -p autoware_data # Data directory
    mkdir -p autoware_map # Map directory
    ```

2. Clone [`autowarefoundation/autoware`](https://github.com/autowarefoundation/autoware) repository and move to the directory:

    ```bash
    git clone https://github.com/autowarefoundation/autoware.git
    cd autoware
    ```

3. Execute the `setup-dev-env.sh` script to install all required components:

    > [!NOTE]
    > GPU acceleration is required for some features such as object detection and traffic light detection/classification. For details of how to enable these features without a GPU, refer to the [Running Autoware without CUDA](https://autowarefoundation.github.io/autoware-documentation/main/how-to-guides/others/running-autoware-without-cuda/) guide.

    **[OPTION 1]** Install with NVIDIA GPU support:
    ```bash
    sudo ./setup-dev-env.sh -y docker
    ```

    **[OPTION 2]** Install without NVIDIA GPU support:
    ```bash
    sudo ./setup-dev-env.sh -y --no-nvidia docker
    ```

4. Download the required artifacts for Autoware stack:

    ```bash
    ./setup-dev-env.sh -y download_artifacts --data-dir ~/Autoware_WS/autoware-data

    gdown -O ~/Autoware_WS/autoware_map/ 'https://docs.google.com/uc?export=download&id=1499_nsbUbIeturZaDj7jhUownh5fvXHd'

    unzip -d ~/Autoware_WS/autoware_map ~/Autoware_WS/autoware_map/sample-map-planning.zip

    gdown -O ~/Autoware_WS/autoware_map/ 'https://github.com/tier4/AWSIM/releases/download/v1.1.0/nishishinjuku_autoware_map.zip'

    unzip -d ~/Autoware_WS/autoware_map ~/Autoware_WS/autoware_map/nishishinjuku_autoware_map.zip
    ```

    > [!NOTE]
    > The compressed versions of the map files can be deleted after their successful extraction.

## Validation:

Execute the `run.sh` script to validate the successful installation of the required components by launching the `planning_simulator` with the `sample-map-planning` map, `sample_vehicle` vehicle, and `sample_sensor_kit` sensor kit:

> [!NOTE]
> Use `--no-nvidia` argument to run without NVIDIA GPU support, and `--headless` argument to run without display, i.e., no RViz visualization.

```bash
cd ~/Autoware_WS/autoware

sudo ./docker/run.sh --map-path ../autoware_map/sample-map-planning --data-path ../autoware_data ros2 launch autoware_launch planning_simulator.launch.xml map_path:=/autoware_map vehicle_model:=sample_vehicle sensor_model:=sample_sensor_kit
```

## Development:

1. From the host workstation, execute the `run.sh` script with the `--devel` argument:

    ```bash
    cd ~/Autoware_WS/autoware

    sudo ./docker/run.sh --map-path ../autoware_map/sample-map-planning --data-path ../autoware_data --workspace . --devel
    ```

    > [!NOTE]
    > By default workspace mounted on the container will be current directory (`pwd`), but can be changed by `--workspace path/to/workspace` argument. For development environments without NVIDIA GPU support use `--no-nvidia` argument.

2. Once inside the container, check if the `workspace` has been mounted correctly:

    ```bash
    cd /workspace

    ls
    ```

    The contents of this `workspace` directory should appear like `autoware` on your host workstation.

3. From within the container, with `workspace` being the current working directory, set up and build the workspace for development:

    ```bash
    mkdir src

    vcs import src < autoware.repos

    sudo apt update && sudo apt upgrade

    rosdep update
    
    rosdep install -y --from-paths src --ignore-src --rosdistro $ROS_DISTRO

    colcon build --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Release

    cd /workspace/src/universe/autoware_universe/autodrive

    pip3 install -r requirements.txt

    sudo apt update && apt install -y --no-install-recommends ros-$ROS_DISTRO-tf-transformations ros-$ROS_DISTRO-imu-tools

    sudo apt update --fix-missing && apt install -y xvfb ffmpeg libgdal-dev libsm6 libxext6
    ```

    > [!NOTE]
    > - You can ignore the `Invalid version` errors (if any) during `rosdep install` process.
    > - You can ignore the `stderr` warnings (if any) during the `colcon build` process.
    > - For more advanced build options, refer to the [colcon documentation](https://colcon.readthedocs.io).
    > - Use `colcon build --packages-select <package_name> --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Release` to re-build only specific packages instead of (re)building the entire workspace to avoid large (re)build times.
    > - If any changes to files of a particular package don't seem to take effect, delete the respective package directories from the `install` and `build` directories and rebuild the respective package using `--packages-select` option.

4. In order to get out from the container to the host workstation, use the `exit` command from within the container:

    ```bash
    exit
    ```

5. In order to transfer the ownership of the workspace to the current user (if not already done), use the `chown` command from the host workstation:

    ```bash
    sudo chown -R $USER ~/Autoware_WS
    ```

6. After the initial setup, in order to update the workspace, use the `git pull` command from the host workstation:

    ```bash
    cd ~/Autoware_WS/autoware

    git pull

    sudo ./docker/run.sh --map-path ../autoware_map/sample-map-planning --data-path ../autoware_data --workspace . --devel
    ```

    Then, use the `vcs import` command from within the container abd rebuild the stack:
    
    ```bash
    cd /workspace

    git config --global --add safe.directory '*'
    
    vcs import src < autoware.repos

    vcs pull src
    
    sudo apt update && sudo apt upgrade
    
    rosdep update
    
    rosdep install -y --from-paths src --ignore-src --rosdistro $ROS_DISTRO

    colcon build --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Release
    ```

    It might be the case that dependencies imported via `vcs import` have been moved/removed. VCStool does not currently handle those cases, so if builds fail after `vcs import`, cleaning and re-importing all dependencies may be necessary:

    ```bash
    rm -rf src/*
    vcs import src < autoware.repos
    ```

    In order to cleanly rebuild, remove the `build`, `install`, and `log` directories.

    ```bash
    cd /workspace
    sudo rm -r build install log
    ```

## Contribution:

1. In order to commit the codebase to [GitHub](https://github.com), either use [GitHub Desktop](https://github.com/shiftkey/desktop?tab=readme-ov-file#installation-via-package-manager) or use the following commands:

    ```bash
    cd ~/Autoware_WS/autoware/src/universe/autoware_universe

    git add .

    git commit -m "COMMIT_MESSAGE"

    git pull
    
    git push
    ```

    > [!NOTE]
    > Do not forget to replace the placeholders `COMMIT_MESSAGE`, `AUTHOR_NAME`, `CONTAINER_ID`, `USERNAME`, `REPOSITORY_NAME`, and `TAG` with actual valid content.

    ```bash
    cd ~/Autoware_WS/autoware/src/universe/autoware_universe

    git add .

    git commit -m "AutoDRIVE-Autoware"

    git pull

    git push
    ```

2. With the container running, execute a new terminal instance (new window/tab) to commit and push the container as an image on [Docker Hub](https://hub.docker.com).

    ```bash
    docker ps -a

    docker commit -m "COMMIT_MESSAGE" -a "AUTHOR_NAME" CONTAINER_ID USERNAME/REPOSITORY_NAME:TAG

    docker login

    docker push USERNAME/REPOSITORY_NAME:TAG
    ```

    > [!NOTE]
    > Do not forget to replace the placeholders `COMMIT_MESSAGE`, `AUTHOR_NAME`, `CONTAINER_ID`, `USERNAME`, `REPOSITORY_NAME`, and `TAG` with actual valid content.

    ```bash
    docker ps -a

    docker commit -m "AutoDRIVE-Autoware" -a "AutoDRIVE Ecosystem" 5db173558138 autodriveecosystem/autodrive_autoware:v0.1.0

    docker login

    docker push autodriveecosystem/autodrive_autoware:v0.1.0
    ```

## Tips:

1. To access the container while it is running, execute the following command in a new terminal window to start a new bash session inside the container:
    ```bash
    docker exec -it <container_name> bash
    ```

2. To exit the bash session(s), simply execute:
    ```bash
    exit
    ```

3. To kill the container, execute the following command:
    ```bash
    docker kill <container_name>
    ```

4. To remove the container, simply execute:
    ```bash
    docker rm <container_name>
    ```

5. Running or caching multiple docker images, containers, volumes, and networks can quickly consume a lot of disk space. Hence, it is always a good idea to frequently check docker disk utilization:
    ```bash
    docker system df
    ```

6. To avoid utilizing a lot of disk space, it is a good idea to frequently purge docker resources such as images, containers, volumes, and networks that are unused or dangling (i.e. not tagged or associated with a container). There are several ways with many options to achieve this, please refer to appropriate documentation. The easiest way (but a potentially dangerous one) is to use a single command to clean up all the docker resources (dangling or otherwise):
    ```bash
    docker system prune -a
    ```

## References:

- For the official Docker installation guide, see [Docker Documentation](https://docs.docker.com/engine/install/ubuntu/#install-using-the-repository), and after installation, follow the [Post-Installation Steps](https://docs.docker.com/desktop/install/ubuntu/) for complete setup.

- For the official NVIDIA Container Toolkit installation guide, see [NVIDIA Documentation](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html). After following the [Installation Steps](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html#with-apt-ubuntu-debian), do not forget to follow the [Configuration Steps](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html#configuring-docker) as well.

- For Autoware's general documentation, see [Autoware Documentation](https://autowarefoundation.github.io/autoware-documentation/).

- For detailed documents of Autoware Universe components, see [Autoware Universe Documentation](https://autowarefoundation.github.io/autoware.universe/).

- For the official Autoware installation guide, see [Autoware Installation Guide](https://autowarefoundation.github.io/autoware-documentation/main/installation/).
  - [Autoware Source Installation](https://autowarefoundation.github.io/autoware-documentation/main/installation/autoware/source-installation/)
  - [Autoware Docker Installation](https://autowarefoundation.github.io/autoware-documentation/main/installation/autoware/docker-installation/)

- For the official Autoware planning simulation guide, see [Planning Simulation Documentation](https://autowarefoundation.github.io/autoware-documentation/main/tutorials/ad-hoc-simulation/planning-simulation/).