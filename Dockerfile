FROM python:3

RUN pip install --upgrade pip
RUN pip install numpy

ADD bsf-py/./ /tmp/bsfpy
WORKDIR /tmp/bsfpy

RUN python setup.py build_ext -DDEBUG -DBSF_AND install
