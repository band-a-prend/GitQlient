#include "GitConfig.h"

#include <GitBase.h>
#include <GitCloneProcess.h>

bool GitUserInfo::isValid() const
{
   return !mUserEmail.isNull() && !mUserEmail.isEmpty() && !mUserName.isNull() && !mUserName.isEmpty();
}

GitConfig::GitConfig(QSharedPointer<GitBase> gitBase, QObject *parent)
   : QObject(parent)
   , mGitBase(gitBase)
{
}

GitUserInfo GitConfig::getGlobalUserInfo() const
{
   GitUserInfo userInfo;

   const auto nameRequest = mGitBase->run("git config --get --global user.name");

   if (nameRequest.first)
      userInfo.mUserName = nameRequest.second.trimmed();

   const auto emailRequest = mGitBase->run("git config --get --global user.email");

   if (emailRequest.first)
      userInfo.mUserEmail = emailRequest.second.trimmed();

   return userInfo;
}

void GitConfig::setGlobalUserInfo(const GitUserInfo &info)
{
   mGitBase->run(QString("git config --global user.name \"%1\"").arg(info.mUserName));
   mGitBase->run(QString("git config --global user.email %1").arg(info.mUserEmail));
}

GitUserInfo GitConfig::getLocalUserInfo() const
{
   GitUserInfo userInfo;

   const auto nameRequest = mGitBase->run("git config --get --local user.name");

   if (nameRequest.first)
      userInfo.mUserName = nameRequest.second.trimmed();

   const auto emailRequest = mGitBase->run("git config --get --local user.email");

   if (emailRequest.first)
      userInfo.mUserEmail = emailRequest.second.trimmed();

   return userInfo;
}

void GitConfig::setLocalUserInfo(const GitUserInfo &info)
{
   mGitBase->run(QString("git config --local user.name \"%1\"").arg(info.mUserName));
   mGitBase->run(QString("git config --local user.email %1").arg(info.mUserEmail));
}

bool GitConfig::clone(const QString &url, const QString &fullPath)
{
   const auto asyncRun = new GitCloneProcess(mGitBase->getWorkingDir());
   connect(asyncRun, &GitCloneProcess::signalProgress, this, &GitConfig::signalCloningProgress, Qt::DirectConnection);

   QString buffer;
   return asyncRun->run(QString("git clone --progress %1 %2").arg(url, fullPath), buffer);
}

bool GitConfig::initRepo(const QString &fullPath)
{
   return mGitBase->run(QString("git init %1").arg(fullPath)).first;
}

GitExecResult GitConfig::getLocalConfig() const
{
   const auto ret = mGitBase->run("git config --list");

   return ret;
}

GitExecResult GitConfig::getGlobalConfig() const
{
   const auto ret = mGitBase->run("git config --global --list");

   return ret;
}

GitExecResult GitConfig::getRemoteForBranch(const QString &branch)
{
   const auto config = getLocalConfig();

   if (config.success)
   {
      const auto values = config.output.toString().split('\n', QString::SkipEmptyParts);
      const auto configKey = QString("branch.%1.remote=").arg(branch);
      QString configValue;

      for (const auto &value : values)
      {
         if (value.startsWith(configKey))
         {
            configValue = value.split("=").last();
            break;
         }
      }

      if (!configValue.isEmpty())
         return { true, configValue };
   }

   return GitExecResult();
}
