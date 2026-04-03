# ESP32-CAM Auto Cloud Shutter-Yogesh Mene

This project takes a photo every 10 seconds using an ESP32-CAM and uploads it to a cloud server using a standard HTTP `multipart/form-data` POST request.

## Hardware Setup

- **Board:** ESP32-CAM (AI-Thinker module)
- Make sure you provide adequate power (at least 5V 2A recommended) to avoid brownout errors during WiFi transmission or camera initialization.

## Configuration

Before building and flashing, you must update `src/main.cpp` with your specific details:

1. **WiFi Credentials:**

Update the `ssid` and `password` variables with your local WiFi network details.

```cpp
const char* ssid = "Your Wifi Name";
const char* password = "Your Wifi Password";

```

1. **Cloud Endpoint:**

The code uses a standard HTTP POST request. By default, it expects a generic webhook or API endpoint.

**Quick Test using Webhook.site:**

- Go to [webhook.site](https://webhook.site/)
- Copy your unique URL (e.g., `https://webhook.site/12345678-abcd-1234-abcd-123456789abc`)
- Update the code:```cpp

const char* serverName = “webhook.site”;

const char* serverPath = “/12345678-abcd-1234-abcd-123456789abc”;

const int serverPort = 443;

```

## How It Works

1. Connects to your WiFi network.
2. Initializes the OV2640 camera sensor.
3. Every 10 seconds (`captureInterval = 10000`), it grabs a frame from the camera.
4. Opens a secure connection (`WiFiClientSecure`) to the specified server.
5. Uploads the image buffer using an HTTP POST multipart form transmission.
6. Prints the server’s response to the Serial Monitor.

## Custom Cloud Providers

You can easily adapt this to specialized APIs:

- **Cloudinary:** Change `serverName` to `api.cloudinary.com` and update the `serverPath` and POST fields to match their unsigned upload API.
- **Telegram Bot:** Change `serverName` to `api.telegram.org` and POST to `/bot<token>/sendPhoto`.
- **Custom AWS API Gateway / Lambda:** Point it directly to your API endpoint.

```