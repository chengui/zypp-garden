#include <iostream>
#include <vector>
#include <list>

#include <zypp/ZYpp.h>
#include <zypp/ZYppFactory.h>
#include <zypp/PathInfo.h>
#include <zypp/RepoManager.h>
#include <zypp/PoolQuery.h>
#include <zypp/PoolItem.h>
#include <zypp/Package.h>
#include <zypp/Pattern.h>
#include <zypp/ResPool.h>
#include <zypp/ProblemTypes.h>

#include <zypp/repo/RepoType.h>
#include <zypp/sat/Pool.h>

enum RESKIND {PATTERN, PACKAGE};
enum STATUS {INSTALL, UNINSTALL};

bool poolquery_interface(enum RESKIND kind, const char *name, zypp::PoolQuery &pq) {
    zypp::ResKind kinds[2] = {zypp::ResKind::pattern, zypp::ResKind::package};
    zypp::PoolQuery q;
    q.addKind(kinds[kind]);
    q.addAttribute(zypp::sat::SolvAttr::name, name);
    q.setMatchExact();

    if(q.empty())
        return false;

    pq = q;
    return true;
}

bool mark_poolitem(zypp::sat::Solvable solvable, enum STATUS status, bool reset=false) {
    zypp::PoolItem  pitem = zypp::ResPool::instance().find(solvable);
    if(reset)
        pitem.status().resetTransact(zypp::ResStatus::USER);

    switch(status){
        case INSTALL:
            pitem.status().setToBeInstalled(zypp::ResStatus::USER);
            std::cout << pitem.resolvable() << std::endl;
            break;
        case UNINSTALL:
            pitem.status().setToBeUninstalled(zypp::ResStatus::USER);
            std::cout << pitem.resolvable() << std::endl;
            break;
        default:
            std::cout << "" << std::endl;
            return false;
    }
    return true;
}

int main(int argc, char **argv) {
    char cwd[256] = {0};
    char url[1024] = {0};
    char tmp[1024] = {0};
    getcwd(cwd, sizeof(cwd));
    sprintf(url, "dir://%s/base", cwd);
    sprintf(tmp, "%s/tmp", cwd);

    zypp::ZYpp::Ptr zyppPtr = zypp::ZYppFactory::instance().getZYpp();
    zypp::Pathname sysRoot(tmp);

    zyppPtr->initializeTarget( sysRoot, false );

    zypp::RepoManagerOptions moptions(sysRoot);
    zypp::RepoManager manager(moptions);

    // set repository info
    zypp::RepoInfo repo;
    repo.setAlias("tmp");
    repo.setEnabled(true);
    repo.setGpgCheck(false);
    repo.setType(zypp::repo::RepoType::RPMMD);
    repo.addBaseUrl(zypp::Url(url));

    // add repository and refresh
    manager.addRepository(repo);
    manager.refreshMetadata(repo, zypp::RepoManager::RefreshIfNeededIgnoreDelay);
    manager.buildCache(repo, zypp::RepoManager::BuildIfNeeded);
    manager.loadFromCache(repo);

    zypp::sat::Pool pool = zypp::sat::Pool::instance();

    // search pattern 'base'
    zypp::PoolQuery ptnquery;
    if(!poolquery_interface(PATTERN, "base", ptnquery)) {
        std::cout << "\npattern base no found" << std::endl;
        return -1;
    }

    // mark pattern 'base' to be installed
    mark_poolitem(*ptnquery.begin(), INSTALL);

    // search package 'A'
    zypp::PoolQuery pkgquery;
    if(!poolquery_interface(PACKAGE, "A", pkgquery)) {
        std::cout << "\npackage A no found" << std::endl;
        return -1;
    }

    // mark package 'A' to be uninstalled
    mark_poolitem(*pkgquery.begin(), UNINSTALL, true);

    // resolve transactionset
    zyppPtr->resolver()->resolvePool();
    zypp::ResolverProblemList probs = zyppPtr->resolver()->problems();
    for (zypp::ResolverProblemList::const_iterator iter = probs.begin(); iter != probs.end(); ++iter)
        std::cout << (*iter) << std::endl;

    // perform transactionset
    zypp::ZYppCommitPolicy policy;
    policy.restrictToMedia (0); // 0 == install all packages regardless to media
    policy.downloadMode (zypp::DownloadInHeaps);
    policy.syncPoolAfterCommit (true);

    // commit the task
   zypp::ZYppCommitResult result = zyppPtr->commit (policy);
   if(result.noError())
       std::cout << "install OK" << std::endl;
   else
       std::cout << "install FAIL" << std::endl;
    
   return 0;
}
