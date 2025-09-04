#ifndef SCHREINBLUETOOTHMANAGER_H
#define SCHREINBLUETOOTHMANAGER_H

#include <Arduino.h>

// Définition de ULONG_MAX si non définie
#ifndef ULONG_MAX
#define ULONG_MAX 0xFFFFFFFFUL
#endif

class SchreinBluetoothManager {
public:
    // Modes de fonctionnement
    enum class Mode {
        CLIENT,     // Mode client - initie les connexions
        SERVER      // Mode serveur - attend les connexions
    };

    // États de connexion Bluetooth
    enum class ConnectionState {
        DISCONNECTED,     // Déconnecté
        CONNECTING,       // En cours de connexion
        CONNECTED,        // Connecté
        ERROR,            // Erreur de connexion
        RETRY_PENDING     // En attente de retry
    };

    // Structure pour la gestion des retry
    struct RetryConfig {
        bool enableConnectionRetry = true;
        bool enableSendRetry = true;
        bool enableATCommandRetry = true;
        
        uint8_t maxConnectionRetries = 3;
        uint8_t maxSendRetries = 2;
        uint8_t maxATRetries = 2;
        
        unsigned long connectionRetryDelay = 5000;  // 5 secondes
        unsigned long sendRetryDelay = 500;         // 0.5 seconde
        unsigned long atRetryDelay = 1000;          // 1 seconde
        
        // Backoff exponentiel
        bool useExponentialBackoff = true;
        float backoffMultiplier = 2.0;
        unsigned long maxBackoffDelay = 30000;      // 30 secondes max
    };

    // Structure pour stocker les informations de retry
    struct RetryContext {
        bool isRetrying = false;
        uint8_t currentAttempt = 0;
        uint8_t maxAttempts = 0;
        unsigned long nextRetryTime = 0;
        unsigned long currentDelay = 0;
        String lastCommand = "";
        String targetAddress = "";
        String expectedResponse = "";
        unsigned long timeout = 0;
        
        void reset() {
            isRetrying = false;
            currentAttempt = 0;
            maxAttempts = 0;
            nextRetryTime = 0;
            currentDelay = 0;
            lastCommand = "";
            targetAddress = "";
            expectedResponse = "";
            timeout = 0;
        }
    };

    SchreinBluetoothManager(Stream &btStream, Mode mode = Mode::SERVER);
    
    // Configuration
    void setMode(Mode newMode);
    Mode getMode() const;
    
    // Configuration du système de retry
    void configureRetry(const RetryConfig &config);
    RetryConfig getRetryConfig() const;
    void enableRetry(bool enable = true);
    void disableRetry();
    
    // Gestion de connexion avec retry
    void begin();
    void end();
    bool connect(String deviceAddress = "");
    void disconnect();
    bool forceConnect(String deviceAddress, bool skipRetry = false);
    ConnectionState getConnectionState() const;
    bool isConnected() const;
    bool isRetrying() const;
    
    // Informations de retry
    uint8_t getCurrentRetryAttempt() const;
    uint8_t getMaxRetryAttempts() const;
    unsigned long getNextRetryTime() const;
    String getRetryStatus() const;
    
    // Mise à jour non bloquante - à appeler dans loop()
    void loop();
    
    // Envoi de données brutes
    bool sendRawData(const String &data);
    bool sendRawDataWithRetry(const String &data);
    
    // Gestion du PIN
    bool setPin(String newPin);
    String getPin();
    bool validatePin(String pin);
    
    // Informations du module
    String getModuleAddress(bool forceRefresh = false);
    String getConnectedDeviceAddress() const;
    String getModuleName(bool forceRefresh = false);
    bool refreshModuleInfo(unsigned long timeout = 5000);
    
    // Callbacks pour les événements
    void onConnect(void (*callback)());
    void onDisconnect(void (*callback)());
    void onError(void (*callback)(String error));
    void onDataReceived(void (*callback)(String data));
    void onRetryAttempt(void (*callback)(uint8_t attempt, uint8_t maxAttempts));
    void onRetryFailed(void (*callback)(String reason));
    void onRetrySuccess(void (*callback)(uint8_t totalAttempts));

private:
    // Configuration et contextes de retry
    RetryConfig retryConfig;
    RetryContext connectionRetryContext;
    RetryContext sendRetryContext;
    RetryContext atRetryContext;

    // Référence au port série utilisé
    Stream &btStream;
    
    // État actuel
    Mode currentMode;
    ConnectionState connectionState;
    
    // Gestion connexion
    String connectedDeviceAddress;
    unsigned long lastConnectionAttempt;
    unsigned long lastSendAttempt;
    const unsigned long CONNECTION_TIMEOUT = 10000; // 10 secondes
    const unsigned long SEND_TIMEOUT = 2000;        // 2 secondes
    
    // Informations du module
    String moduleAddress;
    String moduleName;
    String modulePin;
    unsigned long lastModuleInfoRefresh;
    
    // Callbacks
    void (*onConnectCallback)() = nullptr;
    void (*onDisconnectCallback)() = nullptr;
    void (*onErrorCallback)(String error) = nullptr;
    void (*onDataReceivedCallback)(String data) = nullptr;
    void (*onRetryAttemptCallback)(uint8_t attempt, uint8_t maxAttempts) = nullptr;
    void (*onRetryFailedCallback)(String reason) = nullptr;
    void (*onRetrySuccessCallback)(uint8_t totalAttempts) = nullptr;
    
    // Méthodes de retry
    void processConnectionRetry();
    void processSendRetry();
    void processATRetry();
    bool startConnectionRetry(const String &address);
    bool startSendRetry(const String &data);
    bool startATRetry(const String &command, const String &expectedResponse, unsigned long timeout);
    unsigned long calculateRetryDelay(uint8_t attempt, unsigned long baseDelay);
    void resetAllRetryContexts();
    
    // Méthodes internes
    void changeConnectionState(ConnectionState newState);
    void processBluetoothCommands();
    bool sendATCommand(String command, String expectedResponse = "OK", unsigned long timeout = 1000);
    bool sendATCommandWithRetry(String command, String expectedResponse = "OK", unsigned long timeout = 1000);
    
    // Lecture des réponses AT
    String readATResponse(unsigned long timeout);
    bool waitForResponse(String expectedResponse, unsigned long timeout = 1000);
    String parseMacAddress(String rawResponse);
    
    // Gestion des données entrantes
    void processIncomingData();
};

#endif