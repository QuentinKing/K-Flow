# K-Flow

#### Check out the demo on youtube to see it synced with audio
[![Watch the video](https://img.youtube.com/vi/U6eLqZDPW7A/maxresdefault.jpg)](https://www.youtube.com/watch?v=U6eLqZDPW7A)

### What is it?

K-Flow is a video generation algorithm inspired by the common AI method of k-means clustering. [Using k-means to segment a given image into some set of common colors is nothing new](https://www.youtube.com/watch?v=yR7k19YBqiw), but in this project I manually slow down the convergence rate and arbitrarily pull the cluster centers towards other colors to create a flow between subsequent iterations. Written in C++ and uses some paralleziation per pixel so the computation is relatively fast.

Given some source image, for each frame we run one iteration of the k-means algorithm to generate some set of cluster centers. Instead of taking a large step towards these clusters, we only interpolate towards the new center based on some flow speed variable set in the code. To produce the vivid animations, we define a interval in which we manually discard the computed center and instead replace it with a randomized color value, which we keep until the interval ends. The intervals are computed based on a beats-per-minute value so that the output can be synced nicely with any song.

The only dependency is the C++ version of OpenCV in order to read and write images. The result of the script will produce a folder of all the frames in png form, if you would like to convert into video I would suggest using ffmpeg and running the following command in the results folder:
```
ffmpeg -i frame%d.png video.avi
```
