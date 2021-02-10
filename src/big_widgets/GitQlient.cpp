#include "GitQlient.h"

#include <InitScreen.h>
#include <GitQlientStyles.h>
#include <GitQlientSettings.h>
#include <QPinnableTabWidget.h>
#include <InitialRepoConfig.h>
#include <GitBase.h>
#include <CreateRepoDlg.h>
#include <ProgressDlg.h>
#include <GitConfig.h>
#include <GitQlientRepo.h>

#include <QMenu>
#include <QEvent>
#include <QProcess>
#include <QTabBar>
#include <QStackedLayout>
#include <QToolButton>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QTabBar>
#include <QPushButton>

#include <QLogger.h>

using namespace QLogger;

GitQlient::GitQlient(QWidget *parent)
   : GitQlient(QStringList(), parent)
{
}

GitQlient::GitQlient(const QStringList &arguments, QWidget *parent)
   : QWidget(parent)
   , mStackedLayout(new QStackedLayout(this))
   , mRepos(new QPinnableTabWidget())
   , mConfigWidget(new InitScreen())

{

   auto repos = parseArguments(arguments);

   QLog_Info("UI", "*******************************************");
   QLog_Info("UI", "*          GitQlient has started          *");
   QLog_Info("UI", QString("*                  %1                  *").arg(VER));
   QLog_Info("UI", "*******************************************");

   setStyleSheet(GitQlientStyles::getStyles());

   const auto homeMenu = new QPushButton();
   const auto menu = new QMenu(homeMenu);

   homeMenu->setIcon(QIcon(":/icons/burger_menu"));
   homeMenu->setIconSize(QSize(17, 17));
   homeMenu->setToolTip("Options");
   homeMenu->setMenu(menu);
   homeMenu->setObjectName("MainMenuBtn");

   menu->installEventFilter(this);

   const auto open = menu->addAction(tr("Open repo..."));
   connect(open, &QAction::triggered, this, &GitQlient::openRepo);

   const auto clone = menu->addAction(tr("Clone repo..."));
   connect(clone, &QAction::triggered, this, &GitQlient::cloneRepo);

   const auto init = menu->addAction(tr("New repo..."));
   connect(init, &QAction::triggered, this, &GitQlient::initRepo);

   menu->addSeparator();

   GitQlientSettings settings;
   const auto recent = new QMenu("Recent repos", menu);
   const auto recentProjects = settings.getMostUsedProjects();

   for (const auto &project : recentProjects)
   {
      const auto projectName = project.mid(project.lastIndexOf("/") + 1);
      const auto action = recent->addAction(projectName);
      action->setData(project);
      connect(action, &QAction::triggered, this, [this, project]() { openRepoWithPath(project); });
   }

   menu->addMenu(recent);

   const auto mostUsed = new QMenu("Most used repos", menu);
   const auto projects = settings.getRecentProjects();

   for (const auto &project : projects)
   {
      const auto projectName = project.mid(project.lastIndexOf("/") + 1);
      const auto action = mostUsed->addAction(projectName);
      action->setData(project);
      connect(action, &QAction::triggered, this, [this, project]() { openRepoWithPath(project); });
   }

   menu->addMenu(mostUsed);

   mRepos->setObjectName("GitQlientTab");
   mRepos->setStyleSheet(GitQlientStyles::getStyles());
   mRepos->setCornerWidget(homeMenu, Qt::TopLeftCorner);
   connect(mRepos, &QTabWidget::tabCloseRequested, this, &GitQlient::closeTab);
   connect(mRepos, &QTabWidget::currentChanged, this, &GitQlient::updateWindowTitle);

   mStackedLayout->setContentsMargins(QMargins());
   mStackedLayout->addWidget(mConfigWidget);
   mStackedLayout->addWidget(mRepos);
   mStackedLayout->setCurrentIndex(0);

   mConfigWidget->onRepoOpened();

   connect(mConfigWidget, &InitScreen::signalOpenRepo, this, &GitQlient::addRepoTab);

   setRepositories(repos);

   const auto geometry = settings.globalValue("GitQlientGeometry", saveGeometry()).toByteArray();

   if (!geometry.isNull())
      restoreGeometry(geometry);

   const auto gitBase(QSharedPointer<GitBase>::create(""));
   mGit = QSharedPointer<GitConfig>::create(gitBase);

   connect(mGit.data(), &GitConfig::signalCloningProgress, this, &GitQlient::updateProgressDialog,
           Qt::DirectConnection);
   connect(mGit.data(), &GitConfig::signalCloningFailure, this, &GitQlient::showError, Qt::DirectConnection);
}

GitQlient::~GitQlient()
{
   GitQlientSettings settings;
   QStringList pinnedRepos;
   const auto totalTabs = mRepos->count();

   for (auto i = 0; i < totalTabs; ++i)
   {
      if (mRepos->isPinned(i))
      {
         auto repoToRemove = dynamic_cast<GitQlientRepo *>(mRepos->widget(i));
         pinnedRepos.append(repoToRemove->currentDir());
      }
   }

   settings.setGlobalValue(GitQlientSettings::PinnedRepos, pinnedRepos);
   settings.setGlobalValue("GitQlientGeometry", saveGeometry());

   QLog_Info("UI", "*            Closing GitQlient            *\n\n");
}

bool GitQlient::eventFilter(QObject *obj, QEvent *event)
{

   if (const auto menu = qobject_cast<QMenu *>(obj); menu && event->type() == QEvent::Show)
   {
      auto localPos = menu->parentWidget()->pos();
      auto pos = mapToGlobal(localPos);
      menu->show();
      pos.setY(pos.y() + menu->parentWidget()->height());
      menu->move(pos);
      return true;
   }

   return false;
}

void GitQlient::openRepo()
{

   const QString dirName(QFileDialog::getExistingDirectory(this, "Choose the directory of a Git project"));

   if (!dirName.isEmpty())
      openRepoWithPath(dirName);
}

void GitQlient::openRepoWithPath(const QString &path)
{
   QDir d(path);
   addRepoTab(d.absolutePath());
}

void GitQlient::cloneRepo()
{
   CreateRepoDlg cloneDlg(CreateRepoDlgType::CLONE, mGit);
   connect(&cloneDlg, &CreateRepoDlg::signalOpenWhenFinish, this, [this](const QString &path) { mPathToOpen = path; });

   if (cloneDlg.exec() == QDialog::Accepted)
   {
      mProgressDlg = new ProgressDlg(tr("Loading repository..."), QString(), 100, false);
      connect(mProgressDlg, &ProgressDlg::destroyed, this, [this]() { mProgressDlg = nullptr; });
      mProgressDlg->show();
   }
}

void GitQlient::initRepo()
{
   CreateRepoDlg cloneDlg(CreateRepoDlgType::INIT, mGit);
   connect(&cloneDlg, &CreateRepoDlg::signalOpenWhenFinish, this, &GitQlient::openRepoWithPath);
   cloneDlg.exec();
}

void GitQlient::updateProgressDialog(QString stepDescription, int value)
{
   if (value >= 0)
   {
      mProgressDlg->setValue(value);

      if (stepDescription.contains("done", Qt::CaseInsensitive))
      {
         mProgressDlg->close();
         openRepoWithPath(mPathToOpen);

         mPathToOpen = "";
      }
   }

   mProgressDlg->setLabelText(stepDescription);
   mProgressDlg->repaint();
}

void GitQlient::showError(int, QString description)
{
   if (mProgressDlg)
      mProgressDlg->deleteLater();

   QMessageBox::critical(this, tr("Error!"), description);
}

void GitQlient::setRepositories(const QStringList &repositories)
{
   QLog_Info("UI", QString("Adding {%1} repositories").arg(repositories.count()));

   for (const auto &repo : repositories)
      addRepoTab(repo);
}

void GitQlient::setArgumentsPostInit(const QStringList &arguments)
{
   QLog_Info("UI", QString("External call with the params {%1}").arg(arguments.join(",")));

   const auto repos = parseArguments(arguments);

   setRepositories(repos);
}

QStringList GitQlient::parseArguments(const QStringList &arguments)
{

   LogLevel logLevel;
   GitQlientSettings settings;

#ifdef DEBUG
   logLevel = LogLevel::Trace;
#else
   logLevel = static_cast<LogLevel>(settings.globalValue("logsLevel", static_cast<int>(LogLevel::Warning)).toInt());
#endif

   if (arguments.contains("-noLog") || settings.globalValue("logsDisabled", true).toBool())
      QLoggerManager::getInstance()->pause();
   else
      QLoggerManager::getInstance()->overwriteLogLevel(logLevel);

   QLog_Info("UI", QString("Getting arguments {%1}").arg(arguments.join(", ")));

   QStringList repos;
   const auto argSize = arguments.count();

   for (auto i = 0; i < argSize;)
   {
      if (arguments.at(i) == "-repos")
      {
         while (++i < argSize && !arguments.at(i).startsWith("-"))
            repos.append(arguments.at(i));
      }
      else
      {
         if (arguments.at(i) == "-logLevel")
         {
            logLevel = static_cast<LogLevel>(arguments.at(++i).toInt());

            if (logLevel >= QLogger::LogLevel::Trace && logLevel <= QLogger::LogLevel::Fatal)
            {
               const auto logger = QLoggerManager::getInstance();
               logger->overwriteLogLevel(logLevel);

               settings.setGlobalValue("logsLevel", static_cast<int>(logLevel));
            }
         }

         ++i;
      }
   }

   const auto manager = QLoggerManager::getInstance();
   manager->addDestination("GitQlient.log", { "UI", "Git", "Cache" }, logLevel);

   return repos;
}

void GitQlient::addRepoTab(const QString &repoPath)
{
   addNewRepoTab(repoPath, false);
}

void GitQlient::addNewRepoTab(const QString &repoPath, bool pinned)
{
   if (!mCurrentRepos.contains(repoPath))
   {
      QFileInfo info(QString("%1/.git").arg(repoPath));

      if (info.isFile() || info.isDir())
      {
         const auto repoName = repoPath.contains("/") ? repoPath.split("/").last() : "";

         if (repoName.isEmpty())
         {
            QMessageBox::critical(
                this, tr("Not a repository"),
                tr("The selected folder is not a Git repository. Please make sure you open a Git repository."));
            QLog_Error("UI", "The selected folder is not a Git repository");
            return;
         }

         conditionallyOpenPreConfigDlg(repoPath);

         const auto repo = new GitQlientRepo(repoPath);
         const auto index = pinned ? mRepos->addPinnedTab(repo, repoName) : mRepos->addTab(repo, repoName);

         connect(repo, &GitQlientRepo::signalEditFile, this, &GitQlient::signalEditDocument);
         connect(repo, &GitQlientRepo::signalOpenSubmodule, this, &GitQlient::addRepoTab);
         connect(repo, &GitQlientRepo::repoOpened, this, &GitQlient::onSuccessOpen);
         connect(repo, &GitQlientRepo::currentBranchChanged, this, &GitQlient::updateWindowTitle);

         repo->setRepository(repoName);

         if (!repoPath.isEmpty())
         {
            QProcess p;
            p.setWorkingDirectory(repoPath);
            p.start("git rev-parse", { "--show-superproject-working-tree" });
            p.waitForFinished(5000);

            const auto output = p.readAll().trimmed();
            const auto isSubmodule = !output.isEmpty();

            mRepos->setTabIcon(index, QIcon(isSubmodule ? QString(":/icons/submodules") : QString(":/icons/local")));

            QLog_Info("UI", "Attaching repository to a new tab");

            if (isSubmodule)
            {
               const auto parentRepo = QString::fromUtf8(output.split('/').last());

               mRepos->setTabText(index, QString("%1 \u2192 %2").arg(parentRepo, repoName));

               QLog_Info("UI",
                         QString("Opening the submodule {%1} from the repo {%2} on tab index {%3}")
                             .arg(repoName, parentRepo)
                             .arg(index));
            }
         }

         mRepos->setCurrentIndex(index);
         mStackedLayout->setCurrentIndex(1);

         mCurrentRepos.insert(repoPath);
      }
      else
      {
         QLog_Info("UI", "Trying to open a directory that is not a Git repository.");
         QMessageBox::information(
             this, tr("Not a Git repository"),
             tr("The selected path is not a Git repository. Please make sure you opened the correct directory."));
      }
   }
   else
      QLog_Warning("UI", QString("Repository at {%1} already opened. Skip adding it again.").arg(repoPath));
}

void GitQlient::closeTab(int tabIndex)
{
   const auto repoToRemove = dynamic_cast<GitQlientRepo *>(mRepos->widget(tabIndex));

   QLog_Info("UI", QString("Removing repository {%1}").arg(repoToRemove->currentDir()));

   mCurrentRepos.remove(repoToRemove->currentDir());
   repoToRemove->close();

   const auto totalTabs = mRepos->count() - 1;

   if (totalTabs == 0)
   {
      mStackedLayout->setCurrentIndex(0);
      setWindowTitle(QString("GitQlient %1").arg(VER));
   }
}

void GitQlient::restorePinnedRepos()
{
   GitQlientSettings settings;
   const auto pinnedRepos = settings.globalValue(GitQlientSettings::PinnedRepos, QStringList()).toStringList();

   for (auto &repo : pinnedRepos)
      addNewRepoTab(repo, true);
}

void GitQlient::onSuccessOpen(const QString &fullPath)
{
   GitQlientSettings settings;
   settings.setProjectOpened(fullPath);

   mConfigWidget->onRepoOpened();
}

void GitQlient::conditionallyOpenPreConfigDlg(const QString &repoPath)
{
   QSharedPointer<GitBase> git(new GitBase(repoPath));

   GitQlientSettings settings;
   auto maxCommits = settings.localValue(git->getGitDir(), "MaxCommits", -1).toInt();

   if (maxCommits == -1)
   {
      const auto preConfig = new InitialRepoConfig(git, this);
      preConfig->exec();
   }
}

void GitQlient::updateWindowTitle()
{

   if (const auto currentTab = dynamic_cast<GitQlientRepo *>(mRepos->currentWidget()))
   {
      if (const auto repoPath = currentTab->currentDir(); !repoPath.isEmpty())
      {
         const auto currentName = repoPath.split("/").last();
         const auto currentBranch = currentTab->currentBranch();

         setWindowTitle(QString("GitQlient %1 - %2 (%3)").arg(VER, currentName, currentBranch));
      }
   }
}
