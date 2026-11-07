#!/bin/bash
glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv
glslc second.vert -o second_vert.spv
glslc second.frag -o second_frag.spv
