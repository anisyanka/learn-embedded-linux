#!/bin/bash

sudo apt autoremove
sudo apt-get update
sudo apt-get upgrade

sudo apt-get install    bison            \
                        flex             \
                        libncurses5-dev  \
                        libncursesw5-dev \
                        autoconf
