local so = require "csocket"

local sk = so.socket(so.AF_INET, so.SOCK_DGRAM, 0)
sk:setdebug(true)
sk:bind("104.224.137.96", "12222")
sk:connect("104f.224.137.96", "22222")
ret, err = sk:sendto("104.224.137.96", "22222", "hello", 2)
ret, err = sk:sendto("hello", 2, "104.224.137.96", "22222")
print(ret, err)
sk:send("hello")
