## introduction

Here are codes during my vulkan learning.

It is called vulkantest for I am just learning.

It works on Ubuntu. If u use other OS, u will have to rewrite the code.

## list

### template

It is a simple program based on [SaschaWillems's code](https://github.com/SaschaWillems/Vulkan/tree/master/examples/renderheadless). I just seperated the class into several functions.

It is quite easy to render other simple graph. You just need to change the `setVertex` and `setCommand` funtion.

## build&run

To build this project, you should have installed vulkan. If you haven't, watch [here](https://vulkan.lunarg.com/sdk/home).

After that, just run `make` in this directory. And the bin file will be saved in `out/bin`.

And stay in this directory, run     `out/bin/template` or `bash run template`, the result picture will be saved in `out/pic/headless.ppm`.

For the relative path, you shouldn't `cd out/bin`. It will make the program miss the shader file.