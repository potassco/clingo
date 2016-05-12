#!/bin/bash
gource . --output-custom-log - | grep "/potassco/code/trunk/gringo" | gource --log-format custom -
