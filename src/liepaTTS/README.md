Precondition: see [README.md](..) in parent folder.

Prepare build dir:
```
qibuild configure -c atom286 --release
```

Build speech synthesis module LiepaTTS:
```
qibuild make -c atom286
```

Copy speech synthesis module LiepaTTS to robot (where 192.168.1.8 is robot ip):
```
scp build-atom286/sdk/bin/LiepaTTS nao@192.168.1.8:/home/nao/naoqi/lib
```
The same way copy two voice folders `/Edvardas` and `/Aiste` from resources folder to robot's folder `/home/nao/naoqi/lib/LiepaTTSResources/`

Register  LiepaTTS service in `/home/nao/naoqi/preferences/autoload.ini`.
