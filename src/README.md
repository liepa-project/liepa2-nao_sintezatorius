Based on https://developer.softbankrobotics.com/nao6/naoqi-developer-guide/sdks/c-sdk/c-sdk-installation-guide

Prerequisites:
```
$ sudo apt install cmake
$ pip install qibuild --user
$ echo 'PATH=$PATH:$HOME/.local/bin' >> ~/.bashrc
```
Configuration:
```
$ qibuild config --wizard
```
Worktree Initialization:

Create a folder which will be a worktree.
Call it for example: _$HOME/myWorktree_
Go to that folder and initialize worktree:
```
$ qibuild init
```
Installing NAOqi SDK:

Retrieve from SoftBank Robotics Community website:

 - C++ SDK archive naoqi-sdk-2.8.5.x-linux64.tar.gz and extract it to
   folder for example: _$HOME/myWorktree/naoqi-sdk-linux64_ 
 - Cross toolchain ctc-linux64-atom-2.8.6.x-xxxxxxxx_xxxxxx.zip and extract it
   to folder for example: _$HOME/myWorktree/ctc-linux64-atom_
   
Create toolchain and setup build configuration:
```
$ qitoolchain create atom286 ctc-linux64-atom/toolchain.xml
$ qibuild add-config atom286 --toolchain atom286
```

Next go to dir [liepaTTS](./liepaTTS) and see README for more instructions.
