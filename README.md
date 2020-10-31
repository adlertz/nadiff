## About

A side-by-side git diff viewer for a VT100 compatible linux terminal.

NOTE: Tabs are displayed as tilde with 3 spaces, `~   `.

## Screenshot

![Screenshot of nadiff](screenshot.png?raw=true "Screenshot")

## How to build

    # build
    make

    # build and install to /usr/local/bin/
    sudo make install

    # uninstall
    sudo make uninstall

## How to use

    git diff | nadiff

    git diff --staged | nadiff

    git diff HEAD~1..HEAD | nadiff

    You get the point.

    You can also use 'git-nadiff' which is installed automatically.

    git-nadiff HEAD~1..HEAD


## How to navigate

### Selecting diff
    n  Next diff
    N  Previous diff


### Scrolling in diff views
    d  Up in both views
    c  Down in both views
    w  Left in both views
    e  Right in both views

### Other
    q  Quit
