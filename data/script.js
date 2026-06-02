const allMode = document.querySelectorAll(".sidebar button");
allMode.forEach(function(button){
    button.addEventListener("click",function(){

        allMode.forEach(function(btn){
            btn.classList.remove("button");
        });
        this.classList.add("button");
    });
    
});


let mode = "chat";
document.getElementById("weatherButton").addEventListener("click", ()=>{
    mode = "weather"
    addMessage("The Weather Mode Activated!", "aiInput");
});

document.getElementById("chatButton").addEventListener("click", ()=>{
    mode = "chat"
    addMessage("The Chat Mode Activated!", "aiInput");
});


function addMessage(text,sender){
    const messageContainer = document.querySelector(".messageContainer");
    const messageDiv = document.createElement("div");
    messageDiv.classList.add("message", sender);
    const messageInput = document.createElement("p");
    messageInput.innerText = text;
    messageContainer.appendChild(messageDiv);
    messageDiv.appendChild(messageInput);
    messageContainer.scrollTop = messageContainer.scrollHeight;
}

document.getElementById("sendButton").addEventListener("click",function(){
    let clientInput = document.getElementById("userInput").value;

    if (clientInput.trim()===''){
        alert("please enter a message!");
        return;
    }

    addMessage(clientInput,"userMessage")
    if (mode === "weather"){
        fetchWeather(clientInput);
    }else if (mode === "chat"){
        fetchChatResponse(clientInput);
    }
    document.getElementById("userInput").value = '';
});


document.getElementById("userInput").addEventListener("keydown",function(event){
    if (event.key === "Enter" ){
        let clientInput = document.getElementById("userInput").value;

    if (clientInput.trim()===''){
        alert("please enter a message!");
        return;
    }

    addMessage(clientInput,"userMessage")
    if (mode === "weather"){
        fetchWeather(clientInput);
    }else if (mode === "chat"){
        fetchChatResponse(clientInput);
    }
    document.getElementById("userInput").value = '';

    }
});

function fetchWeather(clientInput){
    fetch("/weather" , {
        method : "post",
        headers : {
            "content-type":"application/json"
        } ,
        body : JSON.stringify({message:clientInput})
    })

    .then(response => response.json())
    .then(data => {
        addMessage(data.response,"aiInput")
    })
    .catch(error => {
        console.log("Error: " ,error)
        addMessage("Failed to retrive weather data!" , "aiInput")
    });
}

function fetchChatResponse(clientInput){
    fetch("/chat",{
        method:"post",
        headers : {
            "content-type" : "application/json"
        },
        body : JSON.stringify({message:clientInput})
    })
    .then(response => response.json())
    .then(data =>{
        addMessage(data.response, "aiInput")
    })
    .catch(error =>{
        console.log("Error: ",error);
        addMessage("Failed to response!", "aiInput")
    })
}
