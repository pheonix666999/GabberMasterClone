# How to Get Pre-Built macOS & Windows Plugins

**No coding required!** This guide shows you how to use GitHub to automatically build the plugin for macOS.

---

## What You'll Get

After following these steps, you'll download:
- **Gabbermaster Clone.vst3** - For Ableton Live, FL Studio, etc.
- **Gabbermaster Clone.component** - For Logic Pro, GarageBand
- **Gabbermaster Clone.app** - Standalone application

---

## Step-by-Step Instructions

### Step 1: Create a GitHub Account

1. Go to **https://github.com**
2. Click **Sign up** and create a free account
3. Verify your email

### Step 2: Create a New Repository

1. Once logged in, click the **+** icon (top right) → **New repository**
2. Fill in:
   - **Repository name**: `gabbermaster-clone`
   - **Description**: (optional)
   - Select **Public** (this gives you unlimited free builds)
3. Click **Create repository**

### Step 3: Upload the Project Files

**IMPORTANT:** You do NOT need to upload the JUCE folder - it will be downloaded automatically!

1. On your new empty repository page, click **"uploading an existing file"** link
2. Open your project folder on your computer
3. Drag and drop these folders/files into the upload area:
   - `.github` folder (IMPORTANT - contains the build script)
   - `GabbermasterClone` folder
   - `Source` folder
   - `.gitignore` file
   - `BUILD_INSTRUCTIONS.md` file
   - `GITHUB_INSTRUCTIONS.md` file

4. Scroll down and click **Commit changes**

### Step 4: Wait for the Build

1. On your repository page, click the **Actions** tab
2. You'll see **"Build Gabbermaster Clone"** running (yellow dot)
3. Wait about **10-15 minutes** for it to finish (green checkmark)

### Step 5: Download Your Plugins

1. Click on the completed workflow run (the one with green checkmark)
2. Scroll down to the **Artifacts** section
3. Click to download:
   - **Gabbermaster-Clone-macOS** - Contains VST3, AU, and Standalone for Mac
   - **Gabbermaster-Clone-Windows** - Contains VST3 and Standalone for Windows

---

## Installing on macOS

### For Ableton Live (VST3)

1. Unzip `Gabbermaster-Clone-macOS.zip`
2. Find `Gabbermaster Clone.vst3` inside
3. Open Finder, press **Cmd + Shift + G**
4. Paste: `~/Library/Audio/Plug-Ins/VST3/`
5. Press Enter, then drag the `.vst3` file into this folder
6. Open **Terminal** (search for it in Spotlight) and paste this command:
   ```
   xattr -cr ~/Library/Audio/Plug-Ins/VST3/Gabbermaster\ Clone.vst3
   ```
7. Press Enter
8. Open Ableton Live → **Preferences** → **Plug-ins** → Click **Rescan**

### For Logic Pro (AU)

1. From the same unzipped folder, find `Gabbermaster Clone.component`
2. Open Finder, press **Cmd + Shift + G**
3. Paste: `~/Library/Audio/Plug-Ins/Components/`
4. Press Enter, then drag the `.component` file into this folder
5. Open **Terminal** and paste:
   ```
   xattr -cr ~/Library/Audio/Plug-Ins/Components/Gabbermaster\ Clone.component
   ```
6. Press Enter
7. Restart Logic Pro

---

## Troubleshooting

### "The build failed" (red X)
- Click on the failed run to see the error
- Make sure you uploaded the `.github` folder (it might be hidden on Mac - press Cmd+Shift+. to show hidden files)

### "Plugin doesn't appear in Ableton"
1. Make sure you ran the `xattr -cr` command
2. Make sure the plugin is in the correct folder
3. Go to Ableton Preferences → Plug-ins → Rescan
4. Restart Ableton

### "macOS says developer cannot be verified"
This is normal for unsigned plugins. The `xattr -cr` command removes this block. If it still happens:
1. Go to **System Preferences** → **Security & Privacy**
2. Click **Allow Anyway** next to the blocked plugin message

### "Where are my downloads?"
- Artifacts are only available for 90 days
- They appear at the bottom of the workflow run page (scroll down!)
- Make sure the build completed successfully (green checkmark)

---

## Rebuilding After Changes

If you need to rebuild (after code changes):

1. Go to your repository on GitHub
2. Click **Actions** tab
3. Click **Build Gabbermaster Clone** on the left
4. Click **Run workflow** button → **Run workflow**
5. Wait for the new build to complete

---

## Summary

| Step | Action |
|------|--------|
| 1 | Create GitHub account |
| 2 | Create new repository |
| 3 | Upload project files (NOT the JUCE folder) |
| 4 | Wait 10-15 min for build |
| 5 | Download from Artifacts |
| 6 | Install and run xattr command |
| 7 | Rescan in your DAW |

That's it! You now have a working macOS VST3 plugin.
