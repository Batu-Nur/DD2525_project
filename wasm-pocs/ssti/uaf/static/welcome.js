// get the query parameter from the request
let username = new URLSearchParams(window.location.search).get("username") ?? "user!";

alert(`Hello ${username}!`)
