# Chronoptics Kea OpenCV Viewer Example 
![Scheme](doc/chronoptics.png)

[Chronoptics](https://www.chronoptics.com/) designs and develops iToF depth cameras, our latest camera [Kea](https://www.chronoptics.com/cameras) is a highly configurable depth camera, that can also run custom code on camera. This is an example of connecting to the camera and displaying the Intensity, Depth, RGB and projected RGB from the camera. 

The output types are
- Intensity, the amount of reflected IR light
- Depth, the distance to each pixel 
- RGB, the 2 Megapixel camera 
- RGB_PROJECTED, the RGB projected on the depth image, so a pixel(n,m) in the depth image has the rgb value rgb_projected(n,m).  


## Setup instructions

1. Extract the ToF library 
2. Update line 9 of the CMakeLists.txt to ToF install location   
3. Install OpenCV and update line 11 of CMakeLists.txt, or remove the line


To build:

    make build && cd build 
    cmake ../
    make 

This produces kea_opencv_viewer 

    ./kea_opencv_viewer --list 

Will display all the detected cameras 

    ./kea_opencv_viewer --serial 202002a --dmax 30.0 --bgr --bgr_projected --fps 15.0

Is an example to display from camera 202002a. The output is display in 4 separate windows. To close the windows and stop the program hit 'Esc'. 

![Scheme](doc/output.png)

## Support 
For any issues or support please email me, Refael Whyte, r.whyte@chronoptics.com  

