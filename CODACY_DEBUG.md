# Codacy Integration Troubleshooting

## Issue: Codacy not receiving latest commits

### Quick Fixes

1. **Force trigger analysis**
   ```bash
   git commit --allow-empty -m "trigger: force codacy analysis"
   git push origin main
   ```

2. **Check webhook status**
   - Go to GitHub Settings → Webhooks
   - Look for Codacy webhook
   - Check recent deliveries for errors

3. **Verify Codacy settings**
   - Login to Codacy dashboard
   - Check project integrations
   - Ensure GitHub connection is active

### If webhooks are broken:

1. **Recreate webhook manually**
   - In Codacy: Settings → Integrations
   - Disconnect and reconnect GitHub

2. **Check repository permissions**
   - Ensure Codacy has read access
   - Verify organization permissions if applicable

### Alternative: Manual analysis trigger

If automatic webhooks fail, you can trigger analysis manually:

1. Via Codacy dashboard
2. Via API (if you have tokens)
3. By creating empty commits

### Debugging commands:

```bash
# Check latest commit
git log --oneline -5

# Check remote status
git remote -v

# Verify push was successful
git status
```

---

Created: $(date)
Status: Troubleshooting Codacy integration
