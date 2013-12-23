local server = {}

local codes = {[200] = "OK", ["404"] = "Not found"};
local header = ""
local body = ""

server.port = ""
server.response = nil

server.parseReq = function(req)
	local reqTable = {}
    for line in req:gmatch("[^\r\n]*") do
        if reqTable["Method"] == nil then
            reqTable["Method"] = line:sub(0, line:find(" ")-1)
            reqTable["URL"] = line:sub(line:find(" ")+1, line:find(" ", line:find(" ")+1)-1)
            reqTable["HTTPVer"] = line:sub(line:find(" [^ ]*$")+1)
        else
            for k, v in line:gmatch("([^:]*): ([^\r\n]*)") do reqTable[k] = v end
        end
    end
	return reqTable
end

function server.writeHeader(code, options)
	header = "HTTP/1.1 " .. code .. " " .. codes[code] .. "\r\n"
	for f, v in pairs(options) do
		header = header .. f .. ": " .. v .. "\r\n"
	end
end

function server.writeBody(str)
	body = str
end

function server.create(port_in, res)
	server.response = res
	server.port = server.port .. port_in
end

function server.handleReq(req)
	server.response(server.parseReq(req))
	header = header .. "Date: " .. os.date("!%a, %d %b %Y %X GMT") .. "\r\n"
	return header .. "\r\n" .. body
end

return server
