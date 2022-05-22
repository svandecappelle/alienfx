# To build

Using Cmake
```
mkdir build
cd build
make ../
```

# To launch
```
./bin/controller -k r,g,b -l r,g,b <...>
```

With parameters options:

| Option |                |
|--------|----------------|
| k      | Keyboard color |
| t      | Touchpad color |
| m      | Mediabar color |
| l      | Logo color     |
| s      | Speaker color  |

And value in format "r,g,b". With each color value is an integer between 0 and 15. (0 means no color, 15 means full color)

