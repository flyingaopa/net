local so = require "csocket"

local sk = so.socket(so.AF_INET, so.SOCK_DGRAM, 0)
print(type(sk))
sk:bind("104.224.137.96", "22222")
local r, host, port = sk:recvfrom(1024)
print(r,host, port)
