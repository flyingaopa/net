local so = require "csocket"

local sk = so.socket(so.AF_INET, so.SOCK_DGRAM, 0)
sk:setdebug(true)
print(type(sk))
sk:bind("104.224.137.96", "22222")
while true do
    local r, host, port = sk:recvfrom(1024)
    print(#r, r,host, port)
end
