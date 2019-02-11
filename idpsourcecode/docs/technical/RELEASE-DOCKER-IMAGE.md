# IDP3 Docker:

As of early 2017, idp3 is available on docker as krrkul/idp3.
This image comes packaged with both the idp3 binaries as well as the idp-ide.

## Release Instructions
1. First, `ssh krr@krr-buildbot.cs.kuleuven.be`
2. `cp idp3-release` to go to the directory containing the idp3 dockerfile (a copy of which is located in the docs/technical folder).
3. Update the dependencies:
    * The `idp` folder, containing the `idp` binaries, shared folder, etc.
    * The `idp-ide` folder, containing the `idp-ide` binaries and files.
4. Build and upload using the `./generate.sh` script.
   This will build the docker image and try to push it to the repository.
   For this second step to succeed, a docker user from within the krrkul team must be logged in (`docker login`).
   Normally, there should be a user logged in already.
   If not, current krr members with a docker account are Matthias (`mvanderhallen`) and Ingmar (`ingdas`).

## Run Instructions

The first step, before any of the following sections can be used, is the installation
of the docker service daemon on your computer. (https://www.docker.com/products/overview)

### Running the idp-ide

To run the idp-ide, simply:
1. Make sure the docker service is running.
2. Create a folder for idp files.
By default, we will assume this is the folder called `idp` in your home directory.
If you choose to change this folder, you should change the occurrence of `~/idp` in the following steps to the correct path.
3. run

    ```
    docker pull krrkul/idp3:latest; \
    docker run -it --rm --name idp3 -p 4004:4004 -v ~/idp:/root/idp krrkul/idp3:latest
    ```

    The first part will make sure that you are running the newest image, and requires an active internet connection.
4. Visit the idp-ide on http://localhost:4004.

### Running idp

Take care when running idp directly: because it runs in an image, accessing your own files can become a bit tricky.
The following step-by-step lists a few possible ways of handling this.

1. Start again by making sure that the docker service is running.
2. Update to the latest krrkul/idp3 image `docker pull krrkul/idp3:latest;`.
This will require an active internet connection.
3. Depending on what you want, choose a variation on the command `docker run -i --rm --name idp krrkul/idp3:latest idp`.
We will refer to this command as `idp3-docker`.
Ask yourself, do you want to work with multiple files? If so, 4 might be interesting, but be sure to read number 5.
If not, go to 4.
4. The `idp3-docker` command will allow you to run the idp command, reading data from stdin.
You can pipe and redirect streams as you expect:
e.g.
```
cat file.idp | idp3-docker
idp3-docker < file.idp > output 2>&1
```
5. If you want to use multiple files, make sure that
 * They refer to each other using relative paths
 * Determine the 'least common parent folder'
 * Mount this folder to the `/root/idp` folder, by modifying the `idp3-docker` command to include
 `-v local_folder:/root/idp`. i.e.
 `docker run -i --rm --name idp -v local_folder:/root/idp krrkul/idp3:latest idp`
 * Run the `idp3-docker` command, providing all your path arguments as `/root/idp/<...>`
