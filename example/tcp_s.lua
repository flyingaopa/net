local so = require "csocket"

local sk = so.socket(so.AF_INET, so.SOCK_STREAM, 0)
sk:setdebug(true)
print(type(sk))
sk:bind("104.224.137.96", "22222")
sk:listen(10)
local newsk = sk:accept()
print(type(newsk))
while true do
    local r, host, port = newsk:recv(1024)
    print(#r, r,host, port)
end
