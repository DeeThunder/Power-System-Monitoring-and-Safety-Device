/**
 * ESP32 Performance Logger - Google Apps Script Web App
 * Receives POST requests from ESP32 and logs data to appropriate sheet
 *
 * Setup Instructions:
 * 1. Create a Google Sheet with 4 tabs: Latency, Accuracy, TripResponse, SystemEvents
 * 2. Paste this code into Extensions → Apps Script
 * 3. Deploy as Web App (Execute as: Me, Access: Anyone)
 * 4. Copy the deployment URL to your ESP32 config.h
 */

function doPost(e) {
  try {
    // Parse incoming JSON data from ESP32
    var data = JSON.parse(e.postData.contents);

    // Get the active spreadsheet
    var ss = SpreadsheetApp.getActiveSpreadsheet();

    // Get the target sheet by name
    var sheet = ss.getSheetByName(data.sheet);

    // Validate sheet exists
    if (!sheet) {
      return ContentService.createTextOutput(
        JSON.stringify({
          status: "error",
          message: "Sheet not found: " + data.sheet,
        }),
      ).setMimeType(ContentService.MimeType.JSON);
    }

    // Create timestamp for this entry
    var timestamp = new Date();

    // Append row: [Timestamp, ...values from ESP32]
    var row = [timestamp].concat(data.values);
    sheet.appendRow(row);

    // Return success response
    return ContentService.createTextOutput(
      JSON.stringify({
        status: "success",
        message: "Data logged to " + data.sheet,
      }),
    ).setMimeType(ContentService.MimeType.JSON);
  } catch (error) {
    // Return error response
    return ContentService.createTextOutput(
      JSON.stringify({
        status: "error",
        message: error.toString(),
      }),
    ).setMimeType(ContentService.MimeType.JSON);
  }
}

/**
 * Test function to verify script works
 * Run this from the Apps Script editor to test
 *
 * How to test:
 * 1. Select "testPost" from the function dropdown
 * 2. Click the Run button (▶️)
 * 3. Check your "SystemEvents" sheet for a new test row
 */
function testPost() {
  var testData = {
    sheet: "SystemEvents",
    values: ["BOOT", 0, "Test from Apps Script"],
  };

  var mockEvent = {
    postData: {
      contents: JSON.stringify(testData),
    },
  };

  var response = doPost(mockEvent);
  Logger.log(response.getContent());

  // To view the log output:
  // Click "Execution log" at the bottom of the Apps Script editor
}
