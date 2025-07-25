# Testing Checklist for Andromeda GitHub Automation

## 📋 Pre-Testing Setup

- [ ] All files committed and pushed to GitHub
- [ ] Repository is public (or you have proper access)
- [ ] GitHub Actions are enabled in repository settings

## 🐛 Issue Templates Testing

### Test Bug Report Template
- [ ] Navigate to Issues → New Issue
- [ ] Verify "Bug Report" template appears
- [ ] Click "Get started" for Bug Report
- [ ] Check pre-filled content includes:
  - [ ] "Note" about showcase mode
  - [ ] Bug description fields
  - [ ] System information section
  - [ ] Community discussion note
- [ ] Verify labels are auto-assigned (bug, needs-triage)

### Test Feature Request Template
- [ ] Verify "Feature Request" template appears
- [ ] Click "Get started" for Feature Request
- [ ] Check pre-filled content includes:
  - [ ] Note about showcase mode
  - [ ] Problem description
  - [ ] Solution description
  - [ ] Community implementation section
- [ ] Verify labels are auto-assigned (enhancement)

### Test Question Template
- [ ] Verify "Question" template appears
- [ ] Click "Get started" for Question
- [ ] Check pre-filled content includes:
  - [ ] Note about showcase mode
  - [ ] Question format
  - [ ] Documentation checklist
  - [ ] Community help section
- [ ] Verify labels are auto-assigned (question)

## 🤖 Auto-Response Testing

### Test Issue Auto-Response
- [ ] Create a test issue using any template
- [ ] Submit the issue
- [ ] Wait 1-2 minutes
- [ ] Check for auto-comment with content:
  - [ ] "Thank you for your interest in Andromeda! 🚀"
  - [ ] Mention of showcase mode
  - [ ] Community encouragement
  - [ ] Star emoji at the end

### Test PR Auto-Response
- [ ] Create a new branch: `git checkout -b test-automation`
- [ ] Make a small change (e.g., add a line to README)
- [ ] Commit and push: 
  ```bash
  git add .
  git commit -m "test: automation testing"
  git push origin test-automation
  ```
- [ ] Create Pull Request on GitHub
- [ ] Wait 1-2 minutes
- [ ] Check for auto-comment with content:
  - [ ] "Thank you for your contribution to Andromeda! 🎉"
  - [ ] Note about showcase mode
  - [ ] Suggestions for community involvement

## 🔍 Actions Workflow Testing

### Check Workflow Execution
- [ ] Go to Actions tab in GitHub
- [ ] Verify "Auto Response" workflow appears
- [ ] Check recent runs show "success" status
- [ ] Click on a run to see execution details
- [ ] Verify both jobs ran successfully:
  - [ ] "Auto comment on issues" (when issue created)
  - [ ] "Auto comment on pull requests" (when PR created)

### Debug Failed Workflows
If workflows fail:
- [ ] Check Actions tab for error messages
- [ ] Verify YAML syntax in auto-response.yml
- [ ] Check repository permissions:
  - [ ] Settings → Actions → General → Workflow permissions
  - [ ] Should be "Read and write permissions"

## ✅ Success Criteria

All tests pass when:
- [ ] Issue templates display correctly with proper formatting
- [ ] Auto-responses appear within 2 minutes of creating issues/PRs
- [ ] Workflow runs show green checkmarks in Actions tab
- [ ] Response content matches expected templates
- [ ] Labels are automatically assigned to issues

## 🛠️ Manual Test Commands

```bash
# Check file structure
find .github -type f -name "*.yml" -o -name "*.md"

# Validate YAML syntax (if you have yamllint installed)
yamllint .github/workflows/auto-response.yml

# Check git status before testing
git status
git log --oneline -5
```

## 📝 Test Notes

Date: ___________
Tester: ___________

**Issues Found:**
- 
- 
- 

**Working Features:**
- 
- 
- 

**Next Steps:**
- 
- 
- 
