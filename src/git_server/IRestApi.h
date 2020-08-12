#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2020  Francesc Martinez
 **
 ** LinkedIn: www.linkedin.com/in/cescmm/
 ** Web: www.francescmm.com
 **
 ** This program is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation; either
 ** version 2 of the License, or (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 ** Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public
 ** License along with this library; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***************************************************************************************/

#include <Milestone.h>
#include <Label.h>
#include <PullRequest.h>

#include <QObject>
#include <QMap>
#include <QNetworkRequest>

class QNetworkAccessManager;
class QNetworkReply;

namespace GitServer
{

struct Issue;

struct ServerAuthentication
{
   QString userName;
   QString userPass;
   QString endpointUrl;
};

class IRestApi : public QObject
{
   Q_OBJECT

signals:
   /**
    * @brief connectionTested Signal triggered when the connection to the remote Git server succeeds.
    */
   void connectionTested();
   /**
    * @brief labelsReceived Signal triggered after the labels are received and processed.
    * @param labels The processed lables.
    */
   void labelsReceived(const QVector<Label> &labels);
   /**
    * @brief milestonesReceived Signal triggered after the milestones are received and processed.
    * @param milestones The processed milestones.
    */
   void milestonesReceived(const QVector<Milestone> &milestones);
   /**
    * @brief issueCreated Signal triggered when an issue has been created.
    * @param url The url of the issue.
    */
   void issueCreated(QString url);
   /**
    * @brief issueUpdated Signal triggered when an issue has been updated.
    */
   void issueUpdated();
   /**
    * @brief issuesReceived Signal triggered when the issues has been received.
    * @param issues The list of issues.
    */
   void issuesReceived(const QVector<Issue> &issues);

   /**
    * @brief pullRequestsReceived Signal triggered when the pull requests has been received.
    * @param prs The list of prs.
    */
   void pullRequestsReceived(const QVector<PullRequest> &prs);
   /**
    * @brief pullRequestCreated Signal triggered when a pull request has been created.
    * @param url The url of the pull request.
    */
   void pullRequestCreated(QString url);
   /**
    * @brief pullRequestsReceived Signal triggered when the pull requests are received and processed.
    * @param prs The list of pull requests ordered by SHA.
    */
   void pullRequestsStateReceived(QMap<QString, PullRequest> prs);
   /**
    * @brief pullRequestMerged Signal triggered when the pull request has been merged.
    */
   void pullRequestMerged();
   /**
    * @brief errorOccurred Signal triggered when an error happened.
    * @param errorStr The error in string format.
    */
   void errorOccurred(const QString &errorStr);

   /**
    * @brief commentsReceived Signal triggered when the comments for an issue has been received.
    * @param issue The issue with the comments.
    */
   void commentsReceived(const Issue &issue);

   /**
    * @brief reviewsReceived Signal triggered when the reviews for a Pull Request has been received.
    * @param pr The Pull Request with the reviews.
    */
   void reviewsReceived(const PullRequest &pr);

   /**
    * @brief paginationPresent Signal triggered when the issues or pull requests are so many that they are sent
    * paginated.
    * @param current The current page.
    * @param next The next page.
    * @param total The total of pages.
    */
   void paginationPresent(int current, int next, int total);

public:
   explicit IRestApi(const ServerAuthentication &auth, QObject *parent = nullptr);
   virtual ~IRestApi() = default;

   static QJsonDocument validateData(QNetworkReply *reply, QString &errorString);

   /**
    * @brief testConnection Tests the connection agains the server.
    */
   virtual void testConnection() = 0;
   /**
    * @brief createIssue Creates a new issue in the remote Git server.
    * @param issue The informatio of the issue.
    */
   virtual void createIssue(const Issue &issue) = 0;
   /**
    * @brief updateIssue Updates an existing issue or pull request, if it doesn't exist it reports an error.
    * @param issueNumber The issue number to update.
    * @param issue The updated information of the issue.
    */
   virtual void updateIssue(int issueNumber, const Issue &issue) = 0;
   /**
    * @brief createPullRequest Creates a pull request in the remote Git server.
    * @param pullRequest The information of the pull request.
    */
   virtual void createPullRequest(const PullRequest &pullRequest) = 0;
   /**
    * @brief requestLabels Requests the labels to the remote Git server.
    */
   virtual void requestLabels() = 0;
   /**
    * @brief requestMilestones Requests the milestones to the remote Git server.
    */
   virtual void requestMilestones() = 0;
   /**
    * @brief requestIssues Requests the issues to the remote Git server.
    */
   virtual void requestIssues(int page = -1) = 0;

   /**
    * @brief requestPullRequests Requests the pull request to the remote Git server.
    */
   virtual void requestPullRequests(int page = -1) = 0;
   /**
    * @brief requestPullRequestsState Requests the pull request state to the remote Git server.
    */
   virtual void requestPullRequestsState() = 0;
   /**
    * @brief mergePullRequest Merges a pull request into the destination branch.
    * @param number The number of the pull request.
    * @param data Byte array in JSON format with the necessary data to merge the pull request.
    */
   virtual void mergePullRequest(int number, const QByteArray &data) = 0;

   /**
    * @brief requestComments Requests all the comments of an issue. This doesn't get the reviews and coments on reviews
    * for a pull request.
    * @param issue The issue number to query.
    */
   virtual void requestComments(const Issue &issue) = 0;

   /**
    * @brief requestReviews Requests all the reviews in a Pull Requests.
    * @param pr The Pull Request to query.
    */
   virtual void requestReviews(const PullRequest &pr) = 0;

protected:
   QNetworkAccessManager *mManager = nullptr;
   ServerAuthentication mAuth;

   /**
    * @brief createRequest Creates a request to be consumed by the Git remote server.
    * @param page The destination page of the request.
    * @return Returns a QNetworkRequest object with the configuration needed by the server.
    */
   virtual QNetworkRequest createRequest(const QString &page) const = 0;
};

}
