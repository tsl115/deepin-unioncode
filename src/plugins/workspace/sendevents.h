#ifndef SENDEVENTS_H
#define SENDEVENTS_H

#include <QString>

class SendEvents final
{
    SendEvents() = delete;
    SendEvents(const SendEvents &) = delete;
public:
    static void generateStart(const QString &buildSystem, const QString &projectPath, const QString &targetPath);
    static void gengrateEnd(const QString &buildSystem,const QString &projectPath, const QString &targetPath);
};

#endif // SENDEVENTS_H
