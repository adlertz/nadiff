## About

A side-by-side git diff viewer for a VT100 compatible linux terminal.

NOTE: Tabs are displayed as `~   `

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


### Scrolling
    a  Up in left diff view
    z  Down in left diff view

    s  Up in right diff view
    x  Down in right diff view

    d  Up in both diff views
    c  Down in both diff views
    w  Left in both diff views
    e  Right in both diff views

### Quit
    q  Quit
