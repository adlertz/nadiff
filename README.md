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

    n Next diff
    N Previous diff
    a Scroll up in left diff view
    z Scroll down in left diff view
    s Scroll up in right diff view
    x Scroll down in right diff view
    d Scroll up in both left and right diff view
    c Scroll down in both left and right diff view
    w Scroll left in both left and right diff view
    e Scroll right in both left and right diff view
    q Quit
