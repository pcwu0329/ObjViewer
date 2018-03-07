# ObjViewer
**Authors:** [Po-Chen Wu](http://media.ee.ntu.edu.tw/personal/pcwu/)

ObjViewer is a C++ program used for viewing **.OBJ** 3D model files. An .OBJ file can be accompanied by **.MTL** texture files. A .MTL file can be accompanied by other image files.

![alt tag](https://github.com/pcwu0329/ObjViewer/blob/master/image/ObjViewer.png)

### System Requirements for Using ObjViewer

The system requirements for ObjViewer are shown below.

##### a. Supported Operating Systems and Architectures
* Windows 10 (x64)
* (Other OS may still be applicable)

##### b. Software Requirements
* Visual Studio 2015 (or later) ([download](https://www.visualstudio.com/))
* OpenCV 3.2.0 (or later) ([download](https://opencv.org/releases.html))
* wxWidgets 3.1.0 (or later) ([download](https://www.wxwidgets.org/))
* Eigen 3.2.9 (or later) ([download](http://eigen.tuxfamily.org/index.php?title=Main_Page))

### Program Description
ObjViewer is used for viewing **.OBJ** 3D model files. Users can also **generate image sequences** with a 3D model rendered according to specified poses.

### Program Setup
* Set Include Directories

![alt tag](https://github.com/pcwu0329/ObjViewer/blob/master/image/IncDir.png)

* Set Library Directories

![alt tag](https://github.com/pcwu0329/ObjViewer/blob/master/image/LibDir.png)

* Set Environment Variables

![alt tag](https://github.com/pcwu0329/ObjViewer/blob/master/image/EnvVar.png)

* Set Additional Dependencies

![alt tag](https://github.com/pcwu0329/ObjViewer/blob/master/image/AddDep.png)

### Generate Image Sequences

![alt tag](https://github.com/pcwu0329/ObjViewer/blob/master/image/Batch.png)

For image sequence generation, the batch file text content should be

####<model> <image> <camera> <poses> <blur> <noise> <output>
* **<model>**: OBJ model file
* **<image>**: Background image file
* **<camera>**: Camera parameter file
* **<poses>**: Poses file
* **<blur>**: Sigma of Gaussian blur kernel
* **<noise>**: Variance of Gaussian noise
* **<output>**: Output directory");
