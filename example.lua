server = require("server")

server.response = function(req)
	server.writeHeader(200, {["Content-Length"] = 12, ["Connection"] = "close", ["Content-Type"] = "text/html"})
	server.writeBody("Hello world!")
end
