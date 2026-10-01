const fs = require("fs");
const http = require("http");
const serve = require("./serve");

console.log(`kioskcnc server starting up...`);
let port = 8080;
let htdocs = "";
for (let i=2; i<process.argv.length; i++) {
  const arg = process.argv[i];
  if (arg.startsWith("--port=")) port = +arg.substr(7);
  else if (arg.startsWith("--htdocs=")) htdocs = arg.substr(9);
  else throw new Error(`Unexpected argument ${JSON.stringify(arg)}`);
}
if (isNaN(port) || (port < 1) || (port > 0xffff)) throw new Error(`invalid port`);
if (!htdocs) console.log(`WARNING: --htdocs unset. Will not serve the static web app.`);

const server = http.createServer((req, rsp) => serve(req, rsp, htdocs));
server.listen(port, (err) => {
  if (err) throw err;
  console.log(`Listening on port ${port}.`);
});
