#!/bin/bash
glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv
glslc shader.vert -o second.spv
glslc shader.frag -o second.spv
