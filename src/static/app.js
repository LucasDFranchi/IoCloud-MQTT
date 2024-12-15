var button = document.getElementById("connectionButton");

button.addEventListener("click", function() {
    ssid = $("#connected_ssid").val()
    pwd = $("#connected_pwd").val()

    console.log(ssid)
    console.log(pwd)

    $.ajax ({
        url: "/wifiCredentials.json",
        dataType: "json",
        method: "POST",
        cache: false,
        headers: {"my-connected-ssid": ssid, "my-connected-pwd": pwd},
        data: {"timestamp": Date.now()}
    });
});