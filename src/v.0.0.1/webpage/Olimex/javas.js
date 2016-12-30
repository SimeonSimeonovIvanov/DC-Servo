/**
 * Determines when a request is considered "timed out"
 */
var timeOutMS = 5000; //ms
 
/**
 * Stores a queue of AJAX events to process
 */
var ajaxList = new Array();

/**
 * Initiates a new AJAX command
 *
 * @param   the url to access
 * @param   the document ID to fill, or a function to call with response XML (optional)
 * @param	true to repeat this call indefinitely (optional)
 * @param   a URL encoded string to be submitted as POST data (optional)
 */
function newAJAXCommand(url, container, repeat, data)
{
	// Set up our object
	var newAjax = new Object();
	var theTimer = new Date();
	newAjax.url = url;
	newAjax.container = container;
	newAjax.repeat = repeat;
	newAjax.ajaxReq = null;
	
	// Create and send the request
	if(window.XMLHttpRequest) {
        newAjax.ajaxReq = new XMLHttpRequest();
        newAjax.ajaxReq.open((data==null)?"GET":"POST", newAjax.url, true);
        newAjax.ajaxReq.send(data);
    // If we're using IE6 style (maybe 5.5 compatible too)
    } else if(window.ActiveXObject) {
        newAjax.ajaxReq = new ActiveXObject("Microsoft.XMLHTTP");
        if(newAjax.ajaxReq) {
            newAjax.ajaxReq.open((data==null)?"GET":"POST", newAjax.url, true);
            newAjax.ajaxReq.send(data);
        }
    }
    
    newAjax.lastCalled = theTimer.getTime();
    
    // Store in our array
    ajaxList.push(newAjax);
}

/**
 * Loops over all pending AJAX events to determine
 * if any action is required
 */
function pollAJAX() {
	
	var curAjax = new Object();
	var theTimer = new Date();
	var elapsed;
	
	// Read off the ajaxList objects one by one
	for(i = ajaxList.length; i > 0; i--)
	{
		curAjax = ajaxList.shift();
		if(!curAjax)
			continue;
		elapsed = theTimer.getTime() - curAjax.lastCalled;
				
		// If we suceeded
		if(curAjax.ajaxReq.readyState == 4 && curAjax.ajaxReq.status == 200) {
			// If it has a container, write the result
			if(typeof(curAjax.container) == 'function'){
				curAjax.container(curAjax.ajaxReq.responseXML.documentElement);
			} else if(typeof(curAjax.container) == 'string') {
				document.getElementById(curAjax.container).innerHTML = curAjax.ajaxReq.responseText;
			} // (otherwise do nothing for null values)
			
	    	curAjax.ajaxReq.abort();
	    	curAjax.ajaxReq = null;

			// If it's a repeatable request, then do so
			if(curAjax.repeat)
				newAJAXCommand(curAjax.url, curAjax.container, curAjax.repeat);
			continue;
		}
		
		// If we've waited over 1 second, then we timed out
		if(elapsed > timeOutMS) {
			// Invoke the user function with null input
			if(typeof(curAjax.container) == 'function'){
				curAjax.container(null);
			} else {
				// Alert the user
				alert("Command failed.\nConnection to development board was lost.");
			}

	    	curAjax.ajaxReq.abort();
	    	curAjax.ajaxReq = null;
			
			// If it's a repeatable request, then do so
			if(curAjax.repeat)
				newAJAXCommand(curAjax.url, curAjax.container, curAjax.repeat);
			continue;
		}
		
		// Otherwise, just keep waiting
		ajaxList.push(curAjax);
	}
	
	// Call ourselves again in 10ms
	setTimeout("pollAJAX()",100);
	
}// End pollAjax
			
/**
 * Parses the xmlResponse returned by an XMLHTTPRequest object
 *
 * @param	the xmlData returned
 * @param	the field to search for
 */
function getXMLValue(xmlData, field) {
	try {
		if(xmlData.getElementsByTagName(field)[0].firstChild.nodeValue)
			return xmlData.getElementsByTagName(field)[0].firstChild.nodeValue;
		else
			return null;
	} catch(err) { return null; }
}

//kick off the AJAX Updater
setTimeout("pollAJAX()",100);

// Parses the xmlResponse from status.xml and updates the status box
function updateStatus(xmlData) {
	// Check if a timeout occurred
	if(!xmlData)
	{
		document.getElementById('display').style.display = 'none';
		document.getElementById('loading').style.display = 'inline';
		return;
	}

	// Loop over all the LEDs
	for(i = 1; i < 4; i++) {
		if(getXMLValue(xmlData, 'led'+i) == '1')
			document.getElementById('led' + i).style.color = '#090';
		else
			document.getElementById('led' + i).style.color = '#ddd';
	}

	// Loop over all the relays
	for(i = 1; i < 3; i++) {
		if(getXMLValue(xmlData, 'r'+i) == '1')
			document.getElementById('r' + i).style.color = '#090';
		else
			document.getElementById('r' + i).style.color = '#ddd';
	}

	
	// Loop over all the buttons
	for(i = 1; i < 4; i++) {
		if(getXMLValue(xmlData, 'btn'+i) == '0')
			document.getElementById('btn' + i).style.color = '#090';
		else
			document.getElementById('btn' + i).style.color = '#ddd';
	}

	// Loop over all the digital inputs
	for(i = 1; i < 3; i++) {
		if(getXMLValue(xmlData, 'din'+i) == '0')
			document.getElementById('din' + i).style.color = '#090';
		else
			document.getElementById('din' + i).style.color = '#ddd';
	}
	
	// Update the POT value
	document.getElementById('pot0').innerHTML = getXMLValue(xmlData, 'pot0');

	// Update the accelerometer value
	document.getElementById('accX').innerHTML = getXMLValue(xmlData, 'accX');
	document.getElementById('accY').innerHTML = getXMLValue(xmlData, 'accY');
	document.getElementById('accZ').innerHTML = getXMLValue(xmlData, 'accZ');
	document.getElementById('accT').innerHTML = getXMLValue(xmlData, 'accT');

}
setTimeout("newAJAXCommand('status.xml', updateStatus, true)",100);

