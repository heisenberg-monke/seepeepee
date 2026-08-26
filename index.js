console.log("Querying /api/search")
fetch("/api/search",
{
    method: 'POST',
    headers:
    {
        'Content-Type': 'text/plain'
    },
    body: "glsl function for linearly interpolation"
})
.then((response) => console.log(response))

async function search(prompt)
{
    const results = document.getElementById("results");

    results.innerHTML = "";

    const response = await fetch("/api/search", 
    {
        method: 'POST',
        headers: {
            'Content-Type': 'text/plain'
        },
        body: prompt
    });

    const json = await response.json();

    results.innerHTML = "";

    for([path, rank] of json)
    {
        let item = document.createElement("span");

        item.appendChild(document.createTextNode(path));
        item.appendChild(document.createElement("br"));

        results.appendChild(item);
    }
}

let query = document.getElementById("query");
let currentSearch = Promise.resolve();

query.addEventListener("keypress", (e) =>
{
    if(e.key == "Enter")
    {
        search(query.value);
        currentSearch.then(() => search(query.value));
    }
});
