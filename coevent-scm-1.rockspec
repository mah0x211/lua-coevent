package = "coevent"
version = "scm-1"
source = {
    url = "git://github.com/mah0x211/lua-coevent.git"
}
description = {
    summary = "cokqueue/coepoll wrapper module",
    homepage = "https://github.com/mah0x211/lua-coevent",
    license = "MIT/X11",
    maintainer = "Masatoshi Teruya"
}
dependencies = {
    "lua >= 5.1"
}
build = {
    type = "command",
    build_command = "autoreconf -ivf && ./configure",
    install_command = "sh ./install.sh",
    install = {
        lua = {
            coevent = "coevent.lua"
        }
    }
}

