const char HTML[] PROGMEM = R"(

<!DOCTYPE html>
<html lang="en">
    <head>
        <meta charset="utf-8"/>
        <title>Splitflap Display</title>
        <link rel="icon" type="image/x-icon" href="https://raw.githubusercontent.com/MaelStudio/Splitflap/refs/heads/main/ESP32/splitflap.ico"/>
        <style>
            html {
                font-size: 24px;
            }

            body {
                font-family: Verdana, Geneva, Tahoma, sans-serif;
                background-color: #f7fafc;
                padding: 1rem 1rem;
                cursor: default;
            }
            .container {
                max-width: 2xl;
                margin: 0 auto;
                padding: 0 0;
            }
            .card {
                max-width: 40rem;
                margin: 0 auto;
                background-color: white;
                border-radius: 0.5rem;
                box-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
                padding: 1rem;
            }
            h1 {
                text-align: center;
                color: #1F2937;
                font-size: 2rem;
                font-weight: bold;
                margin-top: 0.5rem;
            }
            h2 {
                color: #374151;
                font-size: 1.5rem;
                font-weight: semibold;
                margin-bottom: 0.5rem;
            }
            .time-date {
                text-align: center;
                font-size: 1rem;
                color: #374151;
                margin-bottom: 1rem;
            }
            .display {
                text-align: center;
                display: flex;
                gap: 0.5rem;
                background-color: black;
                padding: 0.8rem;
                border-radius: 0.5rem;
                justify-content: center;
                width: fit-content;
                margin: 0 auto;
            }
            .flap {
                font-size: 3.5rem;
                width: 3rem;
                height: 4.5rem;
                background-color: black;
                color: white;
                display: flex;
                align-items: center;
                justify-content: center;
                font-family: Arial, Helvetica, sans-serif;
                border: 1px solid #95989bcc;
            }
            .button-group {
                display: grid;
                grid-template-columns: repeat(2, 1fr);
                gap: 0.3rem;
                margin: 0 auto;
            }
            button {
                padding: 1rem 1rem;
                border: 0px;
                color: white;
                border-radius: 0.2rem;
                cursor: pointer;
                transition: background-color 0.3s;
                width: 100%;
                font-size: 1rem;
            }
            .button-group button {
                background-color: #6b7280;
            }
            .button-group button:hover {
                background-color: #4f596b;
            }
            .button-group button.active {
                background-color: #3b82f6 !important;
            }
            .button-group button.active:hover {
                background-color: #2067da !important;
            }
            .input-group {
                display: flex;
                gap: 0.3rem;
            }
            .input-group input {
                padding: 0.5rem;
                border-radius: 0.2rem;
                border: 1px solid #c9cdd1;
                width:6rem;
                font-size: 1rem;
            }
            .input-group button {
                background-color: #22C45E;
            }
            .input-group button:hover {
                background-color: #059669;
            }
        </style>
    </head>
    <body onload>
        <div class="container">
            <div class="card">
                <h1>Maël's Splitflap<br>Display</h1>
                <div class="time-date" id="timeDate"></div>
                <div class="display">
                    <div class="flap"> </div>
                    <div class="flap"> </div>
                    <div class="flap"> </div>
                    <div class="flap"> </div>
                    <div class="flap"> </div>
                    <div class="flap"> </div>
                </div>
                <div>
                    <h2>Mode</h2>
                    <div class="button-group">
                        <button>LOCAL IP</button>
                        <button>TIME</button>
                        <button>DATE</button>
                        <button>TEMPERATURE</button>
                        <button>SLEEP</button>
                    </div>
                </div>
                <div>
                    <h2>Send Message</h2>
                    <div class="input-group">
                        <input id="msg" maxlength="6" type="text"/>
                        <button id="send">Send</button>
                    </div>
                </div>
            </div>
        </div>
        <script>
            // Date and time
            function updateTimeDate() {
                const now = new Date();
                const timeString = now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
                const dateString = now.toLocaleDateString();
                document.getElementById('timeDate').textContent = `${dateString} | ${timeString}`;
            }
            updateTimeDate();
            setInterval(updateTimeDate, 1000);

            // Mode buttons
            document.querySelectorAll('.button-group button').forEach((button, index) => {
                button.addEventListener('click', function() {
                    fetch(`/mode?id=${index}`);
                });
            });
            
            // Send button
            document.getElementById('send').addEventListener('click', function(){
                const input = document.getElementById('msg');
                const msg = input.value.toUpperCase();
                if (!msg) {
                    alert("Message cannot be empty.");
                    return;
                }
                if (!/^[A-Z0-9:$\- ]*$/.test(msg)) {
                    alert("Message can only contain chars A-Z, 0-9, $, -, :, and space.");
                    return;
                }
                
                input.value = '';
                // Send message to the server as a GET request
                fetch(`/send?msg=${msg}`);
            });
            
            // Also trigger Send with Enter key
            document.getElementById('msg').addEventListener('keydown', function(event) {
                if (event.key === 'Enter') {
                    document.getElementById('send').click();
                }
            });
            
            // SSE for real-time webpage updates
            const eventSource = new EventSource('/events');
            
            // Update displayed message
            eventSource.addEventListener('displayedMsg', function(e) {
                const msg = e.data;
                const displayFlaps = document.querySelectorAll('.flap');
                for (let i = 0; i < displayFlaps.length; i++) {
                    displayFlaps[i].textContent = msg[i] || ' '; // Update the flaps
                }
            });

            // Update current mode
            eventSource.addEventListener('mode', function(e) {
                const mode = e.data;
                const buttons = document.querySelectorAll('.button-group button');
                for (let i = 0; i < buttons.length; i++) {
                    buttons[i].classList.toggle('active', i == mode);
                }
            });
        </script>
    </body>
</html>

)";