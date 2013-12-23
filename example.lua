server = require("server")

server.buildHeader = function(req)
	server.writeHeader(200, {["Content-Length"] = 12, ["Connection"] = "close", ["Content-Type"] = "text/html"})
end
