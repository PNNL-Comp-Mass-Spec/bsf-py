# bsf-py: Python wrapper for Blazing Signature Filter
The Blazing Signature Filter (BSF) is a highly efficient pairwise similarity algorithm which enables extensive data mining within a reasonable amount of time.


## How to install
1. Install python. Python3+ and conda recommended. 
Please refer to https://conda.io/docs/download.html.
2. Clone this repository.
Go to the desired folder and clone it as follows:
```bash
git clone https://github.com/PNNL-Comp-Mass-Spec/bsf-core
```
3. Install BSF package. You can select either -DBSF_XOR or -DBSF_AND as a build option.
```bash
python setup.py build_ext -DDEBUG -DBSF_AND install
```
4. Run the test via jupyter notebook
```bash
jupyter notebook
```
On browsers, you can access the tree GUI. 
```url
http://localhost:8888/
```

## Docker version
Docker is a container platform which enables to deliver the software pipeline as a vitually isolated environment. In other words, without any concerns about the version of c++ compilers and python packages, you can easily build the same environment by running a docker image. You can find more information in [here](https://www.docker.com/what-docker).
Please refer to [this documentation](https://docs.docker.com/get-started/) for getting started and [this user guide](https://docs.docker.com/engine/userguide/) for more details.
1. Install the latest docker in your machine.
Please refer to https://docs.docker.com/engine/installation/.
2. Pull the BSF image.
```bash
docker pull coldfire79/bsf-py
```
3. Run the BSF image.
```bash
docker run -i -t --name bsf coldfire79/bsf-py /bin/bash
```
4. Run python and import BSF.
```bash
root@3d0db695399b:/tmp/bsfpy# python
```
You can see the python prompt. Then you can get started via following the next tutorial example.
```python
import bsf
```
## Tutorial
Please refer to [this tutorial](tutorial/tutorial.ipynb).

## License
[BSD License](LICENSE.txt).
