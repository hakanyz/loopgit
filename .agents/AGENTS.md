
## Git & Branching Rules
- **Asla doğrudan `master` veya `main` dalında çalışma**: Her zaman yapılacak işe uygun yeni bir branch aç veya ilgili feature/fix branch'inde çalış.
- **Onaysız Commit Yasak**: Kullanıcının açık izni ve onayı olmadan KESİNLİKLE `git commit` yapma.
- **Onaysız Push Yasak**: Kullanıcının açık izni ve onayı olmadan KESİNLİKLE `git push` yapma.

## Deployment Rules
- NEVER commit, tag, or push to GitHub (or cut a new version) without EXPLICIT user permission and approval. Always let the user test the changes locally via Qt Creator first.

## Pre-Release Checklist
Before releasing or tagging ANY new version, you MUST rigorously check the following:
1. Verify and update the hardcoded version in `main.cpp` (e.g. `app.setApplicationVersion`) to EXACTLY match the new tag you are about to push.
2. If there are other version strings in the codebase (like `CMakeLists.txt`), verify they are updated as well.
3. Do NOT push the tag until these version strings are updated and committed.
4. If you fail to update the internal version string, the auto-updater will fall into an infinite loop and break the app for all users. Never skip this check.
