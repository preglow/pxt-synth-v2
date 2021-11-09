radio.onReceivedString(function (receivedString) {
    if (receivedString.includes("plonk")) {
        orchestra.setFreq(200)
    }
})
radio.setGroup(1)
basic.forever(function () {
    if (input.buttonIsPressed(Button.A)) {
        orchestra.setFreq(220)
        basic.pause(100)
    }
})
