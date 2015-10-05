package = "coevent"
version = "scm-1"
source = {
    url = "git://github.com/mah0x211/lua-coevent.git"
}
description = {
    summary = "kqueue/epoll event module",
    homepage = "https://github.com/mah0x211/lua-coevent",
    license = "MIT/X11",
    maintainer = "Masatoshi Teruya"
}
dependencies = {
    "lua >= 5.1",
    "reco >= 1.2.1",
    "sentry",
}
build = {
    type = "builtin",
    modules = {
        coevent = "coevent.lua"
    }
}
