#include "SchreinBluetoothManager.h"

SchreinBluetoothManager::SchreinBluetoothManager(Stream &btStream, Mode mode) 
    : btStream(btStream), 
      currentMode(mode),
      connectionState(ConnectionState::DISCONNECTED),
      lastConnectionAttempt(0),
      lastSendAttempt(0),
      lastModuleInfoRefresh(0),
      modulePin("1234") {
    resetAllRetryContexts();
}

void SchreinBluetoothManager::setMode(Mode newMode) {
    if (currentMode != newMode) {
        disconnect();
        currentMode = newMode;
    }
}

SchreinBluetoothManager::Mode SchreinBluetoothManager::getMode() const {
    return currentMode;
}

void SchreinBluetoothManager::configureRetry(const RetryConfig &config) {
    retryConfig = config;
}

SchreinBluetoothManager::RetryConfig SchreinBluetoothManager::getRetryConfig() const {
    return retryConfig;
}

void SchreinBluetoothManager::enableRetry(bool enable) {
    retryConfig.enableConnectionRetry = enable;
    retryConfig.enableSendRetry = enable;
    retryConfig.enableATCommandRetry = enable;
}

void SchreinBluetoothManager::disableRetry() {
    enableRetry(false);
    resetAllRetryContexts();
}

void SchreinBluetoothManager::begin() {
    // Initialisation du module Bluetooth
    delay(1000); // Attendre que le module soit prêt
    
    if (currentMode == Mode::SERVER) {
        // Configuration en mode serveur
        sendATCommandWithRetry("AT+ROLE=0");  // Role: Slave
        sendATCommandWithRetry("AT+CMODE=0"); // Connection mode: specified address
        sendATCommandWithRetry("AT+PSWD=" + modulePin); // PIN
        sendATCommandWithRetry("AT+NAME=Schrein_Device"); // Nom du device
        sendATCommandWithRetry("AT+RESET"); // Redémarrage
    } else {
        // Configuration en mode client
        sendATCommandWithRetry("AT+ROLE=1");  // Role: Master
        sendATCommandWithRetry("AT+CMODE=0"); // Connection mode: specified address
    }
    
    changeConnectionState(ConnectionState::DISCONNECTED);
}

void SchreinBluetoothManager::end() {
    disconnect();
}

bool SchreinBluetoothManager::connect(String deviceAddress) {
    if (currentMode != Mode::CLIENT) {
        if (onErrorCallback) onErrorCallback("Not in client mode");
        return false;
    }
    
    if (deviceAddress != "") {
        connectedDeviceAddress = deviceAddress;
    }
    
    if (connectedDeviceAddress == "") {
        if (onErrorCallback) onErrorCallback("No device address specified");
        return false;
    }
    
    if (retryConfig.enableConnectionRetry) {
        return startConnectionRetry(connectedDeviceAddress);
    } else {
        return forceConnect(connectedDeviceAddress, true);
    }
}

bool SchreinBluetoothManager::forceConnect(String deviceAddress, bool skipRetry) {
    if (!skipRetry && retryConfig.enableConnectionRetry) {
        return connect(deviceAddress);
    }
    
    // Connexion directe sans retry
    if (currentMode != Mode::CLIENT) {
        if (onErrorCallback) onErrorCallback("Not in client mode");
        return false;
    }
    
    if (deviceAddress != "") {
        connectedDeviceAddress = deviceAddress;
    }
    
    if (connectedDeviceAddress == "") {
        if (onErrorCallback) onErrorCallback("No device address specified");
        return false;
    }
    
    changeConnectionState(ConnectionState::CONNECTING);
    lastConnectionAttempt = millis();
    
    // Formater l'adresse (remplacer les : par des ,)
    String formattedAddress = connectedDeviceAddress;
    formattedAddress.replace(":", ",");
    
    // Commande de connexion
    String command = "AT+CONN=" + formattedAddress;
    return sendATCommandWithRetry(command, "CONNECTED", 10000);
}

void SchreinBluetoothManager::disconnect() {
    sendATCommandWithRetry("AT+DISC", "DISC OK", 2000);
    changeConnectionState(ConnectionState::DISCONNECTED);
    connectedDeviceAddress = "";
    resetAllRetryContexts();
}

SchreinBluetoothManager::ConnectionState SchreinBluetoothManager::getConnectionState() const {
    return connectionState;
}

bool SchreinBluetoothManager::isConnected() const {
    return connectionState == ConnectionState::CONNECTED;
}

bool SchreinBluetoothManager::isRetrying() const {
    return connectionRetryContext.isRetrying || 
           sendRetryContext.isRetrying || 
           atRetryContext.isRetrying;
}

uint8_t SchreinBluetoothManager::getCurrentRetryAttempt() const {
    if (connectionRetryContext.isRetrying) return connectionRetryContext.currentAttempt;
    if (sendRetryContext.isRetrying) return sendRetryContext.currentAttempt;
    if (atRetryContext.isRetrying) return atRetryContext.currentAttempt;
    return 0;
}

uint8_t SchreinBluetoothManager::getMaxRetryAttempts() const {
    if (connectionRetryContext.isRetrying) return connectionRetryContext.maxAttempts;
    if (sendRetryContext.isRetrying) return sendRetryContext.maxAttempts;
    if (atRetryContext.isRetrying) return atRetryContext.maxAttempts;
    return 0;
}

unsigned long SchreinBluetoothManager::getNextRetryTime() const {
    unsigned long nextTime = ULONG_MAX;
    if (connectionRetryContext.isRetrying) {
        nextTime = min(nextTime, connectionRetryContext.nextRetryTime);
    }
    if (sendRetryContext.isRetrying) {
        nextTime = min(nextTime, sendRetryContext.nextRetryTime);
    }
    if (atRetryContext.isRetrying) {
        nextTime = min(nextTime, atRetryContext.nextRetryTime);
    }
    return nextTime == ULONG_MAX ? 0 : nextTime;
}

String SchreinBluetoothManager::getRetryStatus() const {
    String status = "Retry Status:\n";
    
    if (connectionRetryContext.isRetrying) {
        status += "Connection: " + String(connectionRetryContext.currentAttempt) + 
                 "/" + String(connectionRetryContext.maxAttempts) + 
                 " (next in " + String((connectionRetryContext.nextRetryTime - millis()) / 1000) + "s)\n";
    }
    
    if (sendRetryContext.isRetrying) {
        status += "Send: " + String(sendRetryContext.currentAttempt) + 
                 "/" + String(sendRetryContext.maxAttempts) + 
                 " (next in " + String((sendRetryContext.nextRetryTime - millis()) / 1000) + "s)\n";
    }
    
    if (atRetryContext.isRetrying) {
        status += "AT Command: " + String(atRetryContext.currentAttempt) + 
                 "/" + String(atRetryContext.maxAttempts) + 
                 " (next in " + String((atRetryContext.nextRetryTime - millis()) / 1000) + "s)\n";
    }
    
    if (!isRetrying()) {
        status += "No active retries\n";
    }
    
    return status;
}

void SchreinBluetoothManager::loop() {
    // Traitement des retry en cours
    processConnectionRetry();
    processSendRetry();
    processATRetry();
    
    // Gérer les timeouts de connexion
    if (connectionState == ConnectionState::CONNECTING && 
        !connectionRetryContext.isRetrying &&
        millis() - lastConnectionAttempt > CONNECTION_TIMEOUT) {
        
        if (retryConfig.enableConnectionRetry) {
            startConnectionRetry(connectedDeviceAddress);
        } else {
            changeConnectionState(ConnectionState::ERROR);
            if (onErrorCallback) onErrorCallback("Connection timeout");
        }
    }
    
    // Traiter les commandes Bluetooth
    processBluetoothCommands();
    
    // Traiter les données utilisateur
    processIncomingData();
}

bool SchreinBluetoothManager::sendRawData(const String &data) {
    if (!isConnected()) {
        if (onErrorCallback) onErrorCallback("Not connected");
        return false;
    }
    
    btStream.print(data);
    btStream.println();
    lastSendAttempt = millis();
    
    return true;
}

bool SchreinBluetoothManager::sendRawDataWithRetry(const String &data) {
    if (!isConnected()) {
        if (onErrorCallback) onErrorCallback("Not connected");
        return false;
    }
    
    if (retryConfig.enableSendRetry) {
        return startSendRetry(data);
    } else {
        return sendRawData(data);
    }
}

bool SchreinBluetoothManager::setPin(String newPin) {
    if (newPin.length() != 4) return false;
    
    String command = "AT+PSWD=" + newPin;
    if (sendATCommand(command, "OK", 2000)) {
        modulePin = newPin;
        return true;
    }
    return false;
}

String SchreinBluetoothManager::getPin() {
    return modulePin;
}

bool SchreinBluetoothManager::validatePin(String pin) {
    return pin.length() == 4;
}

String SchreinBluetoothManager::getModuleAddress(bool forceRefresh) {
    if (moduleAddress == "" || forceRefresh) {
        refreshModuleInfo();
    }
    return moduleAddress;
}

String SchreinBluetoothManager::getConnectedDeviceAddress() const {
    return connectedDeviceAddress;
}

String SchreinBluetoothManager::getModuleName(bool forceRefresh) {
    if (moduleName == "" || forceRefresh) {
        refreshModuleInfo();
    }
    return moduleName;
}

bool SchreinBluetoothManager::refreshModuleInfo(unsigned long timeout) {
    // Sauvegarder l'état actuel
    ConnectionState previousState = connectionState;
    
    // Entrer en mode commande AT
    btStream.println("AT");
    if (!waitForResponse("OK", 1000)) {
        return false;
    }
    
    // Récupérer l'adresse MAC
    btStream.println("AT+ADDR?");
    String response = waitForResponse("+ADDR:", timeout);
    if (response != "") {
        moduleAddress = parseMacAddress(response);
    }
    
    // Récupérer le nom
    btStream.println("AT+NAME?");
    response = waitForResponse("+NAME:", timeout);
    if (response != "") {
        moduleName = response.substring(response.indexOf(":") + 1);
        moduleName.trim();
    }
    
    // Récupérer le PIN
    btStream.println("AT+PSWD?");
    response = waitForResponse("+PSWD:", timeout);
    if (response != "") {
        modulePin = response.substring(response.indexOf(":") + 1);
        modulePin.trim();
    }
    
    // Quitter le mode AT
    btStream.println("AT+RESET");
    delay(1000);
    
    // Restaurer l'état précédent
    changeConnectionState(previousState);
    lastModuleInfoRefresh = millis();
    
    return moduleAddress != "";
}

void SchreinBluetoothManager::onConnect(void (*callback)()) {
    onConnectCallback = callback;
}

void SchreinBluetoothManager::onDisconnect(void (*callback)()) {
    onDisconnectCallback = callback;
}

void SchreinBluetoothManager::onError(void (*callback)(String error)) {
    onErrorCallback = callback;
}

void SchreinBluetoothManager::onDataReceived(void (*callback)(String data)) {
    onDataReceivedCallback = callback;
}

void SchreinBluetoothManager::onRetryAttempt(void (*callback)(uint8_t attempt, uint8_t maxAttempts)) {
    onRetryAttemptCallback = callback;
}

void SchreinBluetoothManager::onRetryFailed(void (*callback)(String reason)) {
    onRetryFailedCallback = callback;
}

void SchreinBluetoothManager::onRetrySuccess(void (*callback)(uint8_t totalAttempts)) {
    onRetrySuccessCallback = callback;
}

void SchreinBluetoothManager::changeConnectionState(ConnectionState newState) {
    if (connectionState != newState) {
        connectionState = newState;
        
        // Appeler les callbacks
        if (newState == ConnectionState::CONNECTED && onConnectCallback) {
            onConnectCallback();
        } else if (newState == ConnectionState::DISCONNECTED && onDisconnectCallback) {
            onDisconnectCallback();
        }
    }
}

String SchreinBluetoothManager::readATResponse(unsigned long timeout) {
    String response = "";
    unsigned long startTime = millis();
    
    while (millis() - startTime < timeout) {
        if (btStream.available()) {
            char c = btStream.read();
            response += c;
            
            if (response.endsWith("\r\n")) {
                response.trim();
                break;
            }
        }
    }
    
    return response;
}

bool SchreinBluetoothManager::waitForResponse(String expectedResponse, unsigned long timeout) {
    unsigned long startTime = millis();
    String response = "";
    
    while (millis() - startTime < timeout) {
        if (btStream.available()) {
            char c = btStream.read();
            response += c;
            
            if (response.indexOf(expectedResponse) >= 0) {
                return true;
            }
            
            // Vérifier les erreurs
            if (response.indexOf("ERROR") >= 0 || response.indexOf("FAIL") >= 0) {
                return false;
            }
        }
    }
    
    return false;
}

String SchreinBluetoothManager::parseMacAddress(String rawResponse) {
    String rawMac = rawResponse;
    
    if (rawMac.startsWith("+ADDR:")) {
        rawMac = rawMac.substring(6);
    }
    rawMac.trim();
    
    // Supprimer les deux-points existants
    String cleanMac;
    for (unsigned int i = 0; i < rawMac.length(); i++) {
        if (rawMac[i] != ':') {
            cleanMac += rawMac[i];
        }
    }
    
    // Formater en XX:XX:XX:XX:XX:XX
    if (cleanMac.length() >= 12) {
        String formattedMac;
        for (int i = 0; i < 12; i++) {
            formattedMac += cleanMac[i];
            if (i % 2 == 1 && i < 11) {
                formattedMac += ":";
            }
        }
        return formattedMac;
    }
    
    return rawMac;
}

void SchreinBluetoothManager::processBluetoothCommands() {
    // Vérifier les réponses Bluetooth (commandes AT)
    while (btStream.available()) {
        String response = readATResponse(100);
        
        if (response.length() > 0) {
            if (response.startsWith("CONNECTED")) {
                changeConnectionState(ConnectionState::CONNECTED);
                resetAllRetryContexts();
            } else if (response.startsWith("DISCONNECTED")) {
                changeConnectionState(ConnectionState::DISCONNECTED);
                resetAllRetryContexts();
            } else if (response.startsWith("ERROR")) {
                changeConnectionState(ConnectionState::ERROR);
                if (onErrorCallback) onErrorCallback(response);
            }
            
            // Transmettre les données reçues au callback
            if (onDataReceivedCallback) {
                onDataReceivedCallback(response);
            }
        }
    }
}

void SchreinBluetoothManager::processIncomingData() {
    // Lire les données utilisateur brutes
    while (btStream.available()) {
        char c = btStream.read();
        
        // Transmettre les données brutes au callback
        static String rawData = "";
        rawData += c;
        
        if (c == '\n' || rawData.length() >= 256) {
            if (onDataReceivedCallback) {
                onDataReceivedCallback(rawData);
            }
            rawData = "";
        }
    }
}

void SchreinBluetoothManager::processConnectionRetry() {
    if (!connectionRetryContext.isRetrying) return;
    
    if (millis() >= connectionRetryContext.nextRetryTime) {
        connectionRetryContext.currentAttempt++;
        
        if (onRetryAttemptCallback) {
            onRetryAttemptCallback(connectionRetryContext.currentAttempt, 
                                 connectionRetryContext.maxAttempts);
        }
        
        // Tentative de connexion
        String formattedAddress = connectionRetryContext.targetAddress;
        formattedAddress.replace(":", ",");
        String command = "AT+CONN=" + formattedAddress;
        
        bool success = sendATCommand(command, "CONNECTED", 10000);
        
        if (success) {
            if (onRetrySuccessCallback) {
                onRetrySuccessCallback(connectionRetryContext.currentAttempt);
            }
            connectionRetryContext.reset();
        } else if (connectionRetryContext.currentAttempt >= connectionRetryContext.maxAttempts) {
            if (onRetryFailedCallback) {
                onRetryFailedCallback("Max connection retries exceeded");
            }
            changeConnectionState(ConnectionState::ERROR);
            connectionRetryContext.reset();
        } else {
            // Programmer le prochain retry
            connectionRetryContext.currentDelay = calculateRetryDelay(
                connectionRetryContext.currentAttempt, 
                retryConfig.connectionRetryDelay
            );
            connectionRetryContext.nextRetryTime = millis() + connectionRetryContext.currentDelay;
        }
    }
}

void SchreinBluetoothManager::processSendRetry() {
    if (!sendRetryContext.isRetrying) return;
    
    if (millis() >= sendRetryContext.nextRetryTime) {
        sendRetryContext.currentAttempt++;
        
        if (onRetryAttemptCallback) {
            onRetryAttemptCallback(sendRetryContext.currentAttempt, 
                                 sendRetryContext.maxAttempts);
        }
        
        // Tentative d'envoi
        btStream.print(sendRetryContext.lastCommand);
        btStream.println();
        lastSendAttempt = millis();
        
        // Pour l'envoi, nous considérons que c'est toujours un succès
        if (onRetrySuccessCallback) {
            onRetrySuccessCallback(sendRetryContext.currentAttempt);
        }
        sendRetryContext.reset();
    }
}

void SchreinBluetoothManager::processATRetry() {
    if (!atRetryContext.isRetrying) return;
    
    if (millis() >= atRetryContext.nextRetryTime) {
        atRetryContext.currentAttempt++;
        
        if (onRetryAttemptCallback) {
            onRetryAttemptCallback(atRetryContext.currentAttempt, 
                                 atRetryContext.maxAttempts);
        }
        
        // Tentative de commande AT
        bool success = sendATCommand(atRetryContext.lastCommand, 
                                   atRetryContext.expectedResponse, 
                                   atRetryContext.timeout);
        
        if (success) {
            if (onRetrySuccessCallback) {
                onRetrySuccessCallback(atRetryContext.currentAttempt);
            }
            atRetryContext.reset();
        } else if (atRetryContext.currentAttempt >= atRetryContext.maxAttempts) {
            if (onRetryFailedCallback) {
                onRetryFailedCallback("Max AT command retries exceeded");
            }
            atRetryContext.reset();
        } else {
            // Programmer le prochain retry
            atRetryContext.currentDelay = calculateRetryDelay(
                atRetryContext.currentAttempt, 
                retryConfig.atRetryDelay
            );
            atRetryContext.nextRetryTime = millis() + atRetryContext.currentDelay;
        }
    }
}

bool SchreinBluetoothManager::startConnectionRetry(const String &address) {
    if (!retryConfig.enableConnectionRetry) return false;
    
    connectionRetryContext.reset();
    connectionRetryContext.isRetrying = true;
    connectionRetryContext.maxAttempts = retryConfig.maxConnectionRetries;
    connectionRetryContext.targetAddress = address;
    connectionRetryContext.currentDelay = retryConfig.connectionRetryDelay;
    connectionRetryContext.nextRetryTime = millis() + connectionRetryContext.currentDelay;
    
    changeConnectionState(ConnectionState::RETRY_PENDING);
    return true;
}

bool SchreinBluetoothManager::startSendRetry(const String &data) {
    if (!retryConfig.enableSendRetry) return false;
    
    sendRetryContext.reset();
    sendRetryContext.isRetrying = true;
    sendRetryContext.maxAttempts = retryConfig.maxSendRetries;
    sendRetryContext.lastCommand = data;
    sendRetryContext.currentDelay = retryConfig.sendRetryDelay;
    sendRetryContext.nextRetryTime = millis() + sendRetryContext.currentDelay;
    
    return true;
}

bool SchreinBluetoothManager::startATRetry(const String &command, const String &expectedResponse, unsigned long timeout) {
    if (!retryConfig.enableATCommandRetry) return false;
    
    atRetryContext.reset();
    atRetryContext.isRetrying = true;
    atRetryContext.maxAttempts = retryConfig.maxATRetries;
    atRetryContext.lastCommand = command;
    atRetryContext.expectedResponse = expectedResponse;
    atRetryContext.timeout = timeout;
    atRetryContext.currentDelay = retryConfig.atRetryDelay;
    atRetryContext.nextRetryTime = millis() + atRetryContext.currentDelay;
    
    return true;
}

unsigned long SchreinBluetoothManager::calculateRetryDelay(uint8_t attempt, unsigned long baseDelay) {
    if (!retryConfig.useExponentialBackoff) {
        return baseDelay;
    }
    
    unsigned long delay = baseDelay;
    for (uint8_t i = 1; i < attempt; i++) {
        delay = (unsigned long)(delay * retryConfig.backoffMultiplier);
        if (delay > retryConfig.maxBackoffDelay) {
            delay = retryConfig.maxBackoffDelay;
            break;
        }
    }
    
    return delay;
}

void SchreinBluetoothManager::resetAllRetryContexts() {
    connectionRetryContext.reset();
    sendRetryContext.reset();
    atRetryContext.reset();
}

bool SchreinBluetoothManager::sendATCommand(String command, String expectedResponse, unsigned long timeout) {
    btStream.println(command);
    
    unsigned long startTime = millis();
    String response = "";
    
    while (millis() - startTime < timeout) {
        if (btStream.available()) {
            char c = btStream.read();
            response += c;
            
            if (response.indexOf(expectedResponse) >= 0) {
                return true;
            }
        }
    }
    
    if (onErrorCallback) onErrorCallback("AT command timeout: " + command);
    return false;
}

bool SchreinBluetoothManager::sendATCommandWithRetry(String command, String expectedResponse, unsigned long timeout) {
    if (!retryConfig.enableATCommandRetry) {
        return sendATCommand(command, expectedResponse, timeout);
    }
    
    for (uint8_t attempt = 1; attempt <= retryConfig.maxATRetries + 1; attempt++) {
        if (attempt > 1) {
            unsigned long waitingtime = calculateRetryDelay(attempt - 1, retryConfig.atRetryDelay);
            delay(waitingtime);
            
            if (onRetryAttemptCallback) {
                onRetryAttemptCallback(attempt - 1, retryConfig.maxATRetries);
            }
        }
        
        bool success = sendATCommand(command, expectedResponse, timeout);
        if (success) {
            if (attempt > 1 && onRetrySuccessCallback) {
                onRetrySuccessCallback(attempt - 1);
            }
            return true;
        }
    }
    
    if (onRetryFailedCallback) {
        onRetryFailedCallback("AT command failed after " + String(retryConfig.maxATRetries) + " retries");
    }
    return false;
}