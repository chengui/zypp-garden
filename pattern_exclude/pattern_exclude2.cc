#include <iostream>

#include <zypp/ZYpp.h>
#include <zypp/ZYppFactory.h>
#include <zypp/RepoManager.h>
#include <zypp/ResPool.h>

#include <zypp/ui/Selectable.h>

using std::endl;

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

    zypp::ResPool pool( zypp::ResPool::instance() );

    zypp::ui::Selectable::Ptr sel;
    sel = zypp::ui::Selectable::get( zypp::ResKind::pattern, "base" );
    if ( ! sel )
        std::cout << "No pattern:base in the pool" << endl;
    else if ( ! sel->setStatus( zypp::ui::S_Install, zypp::ResStatus::USER ) )
        std::cout << "FAILED to set S_Install on " << sel << endl;
    std::cout << sel << endl;

    // Lock package A
    sel = zypp::ui::Selectable::get( zypp::ResKind::package, "A" );
    if ( ! sel )
        std::cout << "No package:A in the pool" << endl;
    else if ( ! sel->setStatus( zypp::ui::S_Taboo, zypp::ResStatus::USER ) )
        std::cout << "FAILED to set S_Taboo on " << sel << endl;
    std::cout << sel << endl;

    // resolve transactionset
    bool depsOK = pool.resolver().resolvePool();
    // resolve
    if ( ! depsOK )
    {
        zypp::ProblemSolutionList solutions;
        zypp::ResolverProblemList problems( pool.resolver().problems() );
        for_( pit, problems.begin(), problems.end() )
        {
            std::cout << "PICK LAST SOLUTION" << endl;
            solutions.push_back( *(--(*pit)->solutions().end()) );
        }
        pool.resolver().applySolutions( solutions );

        std::cout << "SOLVE AGAIN" << endl;
        depsOK = pool.resolver().resolvePool();
        if ( ! depsOK )
            std::cout << "FAILED AGAIN" << endl;
    }

    // perform transactionset
    zypp::ZYppCommitPolicy policy;
    policy.restrictToMedia (0); // 0 == install all packages regardless to media
    policy.downloadMode (zypp::DownloadInHeaps);
    policy.syncPoolAfterCommit (true);

   // commit the task
   zypp::ZYppCommitResult result = zyppPtr->commit (policy);
   if(result.noError())
       std::cout << "install OK" << endl;
   else
       std::cout << "install FAIL" << endl;
   return 0;
}
