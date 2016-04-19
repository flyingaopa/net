local so = require "csocket"
local epoll = require "cepoll"
local sk = so.socket(so.AF_INET, so.SOCK_STREAM, 0)
sk:setdebug(true)
print(sk)
sk:bind("104.224.137.96", "22222")
sk:listen(10)

local ed = epoll.epoll_create()
ed:setdebug(true)

ed:epoll_ctladd(sk:fd(), epoll.EPOLLIN, sk)
while true do
    local ret,res = ed:epoll_wait(10, 0)
    if ret and ret > 0 then
    for k, v in  ipairs(res) do
        if v.fd == sk:fd() then
            local newsk = sk:accept()
            newsk:setdebug(true)
            ed:epoll_ctladd(newsk:fd(), epoll.EPOLLIN, newsk)
        else
            local sock = v.data
            local r, host, port = sock:recv(1024)
            print(#r, r,host, port)
            if #r == 0 then
                ed:epoll_ctldel(sock:fd())
                --sock:close()
                --collectgarbage("collect")
            end
        end
    end 
    end
end
