#ifndef MODEL_H
#define MODEL_H

#include <QString>

struct Model {
    QString name;           // unique display name
    QString provider;       // e.g., "Mistral", "OpenAI", "Ollama"
    QString endpoint;       // full API URL
    QString apiKey;         // optional, may be empty
    QString modelName;      // model identifier used by the provider
    double temperature;     // default 0.7
    int maxTokens;          // default 8192

    // Equality operator for convenience
    bool operator==(const Model &other) const {
        return name == other.name;
    }
};

#endif // MODEL_H