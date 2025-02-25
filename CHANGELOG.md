## v1.3.6
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.3.5...v.1.3.6)

* Fix fullscreen switching [!1895](https://gitlab.com/Remmina/Remmina/merge_requests/1895) *@ToolsDevler*
* Fullscreen fixes [!1897](https://gitlab.com/Remmina/Remmina/merge_requests/1897) *@giox069*
* rdpr channel initialization for special devices sharing. Closes #1955 [!1892](https://gitlab.com/Remmina/Remmina/merge_requests/1892) *@antenore*
* Fixing remminamain destroy issues [!1896](https://gitlab.com/Remmina/Remmina/merge_requests/1896) *@antenore*
* Makes Rmnews modal to avoid that it steals input to the RCW [!1899](https://gitlab.com/Remmina/Remmina/merge_requests/1899) *@antenore*
* Make some cmake targets optional to avoid packaging isssues [!1887](https://gitlab.com/Remmina/Remmina/merge_requests/1887) *@antenore*
* SNAP fixing dependencies for the plugin WWW [!1890](https://gitlab.com/Remmina/Remmina/merge_requests/1890) *@antenore*
* Updating Remmna icon to the yaru/suru icon set. [!1891](https://gitlab.com/Remmina/Remmina/merge_requests/1891) *@antenore*
* Adding harfbuzz headers, closes #1941 [!1894](https://gitlab.com/Remmina/Remmina/merge_requests/1894) *@antenore*
* New stats [!1898](https://gitlab.com/Remmina/Remmina/merge_requests/1898) *@antenore*
* *.md: typo corrections [!1889](https://gitlab.com/Remmina/Remmina/merge_requests/1889) *@Cypresslin*
* Remove G+ from documents [!1888](https://gitlab.com/Remmina/Remmina/merge_requests/1888) *@Cypresslin*

## v1.3.5
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.3.4...v.1.3.5)

**ATTENTION** New dependencies.

* -DWITH_KF5WALLET=ON is needed for the KDE Wallet plugin (and the kf5wallet libs to build it).
* webkit2gtk3 is needed to build the WWW plugin.
* libsodium is needed to buil Remmina

* rcw refactoring [!1815](https://gitlab.com/Remmina/Remmina/merge_requests/1815) *@giox069*
* Remmina files refactoring [!1814](https://gitlab.com/Remmina/Remmina/merge_requests/1814) *@antenore*
* RCW fixes [!1817](https://gitlab.com/Remmina/Remmina/merge_requests/1817) *@giox069*
* Update Czech translation [!1818](https://gitlab.com/Remmina/Remmina/merge_requests/1818) *@asciiwolf*
* Improve English wording [!1819](https://gitlab.com/Remmina/Remmina/merge_requests/1819) *@DevDef*
* Typographic fixes [!1820](https://gitlab.com/Remmina/Remmina/merge_requests/1820) *@DevDef*
* Fix typo timout → timeout [!1821](https://gitlab.com/Remmina/Remmina/merge_requests/1821) *@DevDef*
* Unset “120” (duration before locking) as a translatable string [!1822](https://gitlab.com/Remmina/Remmina/merge_requests/1822) *@DevDef*
* Update Czech translation [!1824](https://gitlab.com/Remmina/Remmina/merge_requests/1824) *@asciiwolf*
* Translate the new strings [!1823](https://gitlab.com/Remmina/Remmina/merge_requests/1823) *@DevDef*
* Patch 1 [!1825](https://gitlab.com/Remmina/Remmina/merge_requests/1825) *@DevDef*
* Unset “300” as a translatable string + grammar fix (some widget*s*) [!1829](https://gitlab.com/Remmina/Remmina/merge_requests/1829) *@DevDef*
* Update German translation [!1828](https://gitlab.com/Remmina/Remmina/merge_requests/1828) *@jweberhofer*
* Updated translation [!1830](https://gitlab.com/Remmina/Remmina/merge_requests/1830) *@DevDef*
* Restore a translation deleted after the removing of a wrong comma in its original string [!1831](https://gitlab.com/Remmina/Remmina/merge_requests/1831) *@DevDef*
* Updated Russian translation (sent from Soltys Sergey by e-mail) [!1832](https://gitlab.com/Remmina/Remmina/merge_requests/1832) *@DevDef*
* Replace non-interpreted HTML lists by text lists [!1833](https://gitlab.com/Remmina/Remmina/merge_requests/1833) *@DevDef*
* Remmina news dialog [!1835](https://gitlab.com/Remmina/Remmina/merge_requests/1835) *@antenore*
* Fixing unlock called in the wrong places [!1836](https://gitlab.com/Remmina/Remmina/merge_requests/1836) *@antenore*
* Show an error dialog instead of crashing, fixes issue #1896 [!1837](https://gitlab.com/Remmina/Remmina/merge_requests/1837) *@giox069*
* Resolve "Remmina profile file always gets overwriten when connecting" [!1838](https://gitlab.com/Remmina/Remmina/merge_requests/1838) *@antenore*
* Translate two new strings [!1839](https://gitlab.com/Remmina/Remmina/merge_requests/1839) *@DevDef*
* WWW plugin - A new plugin to connect to Webbased administration consoles fixes  #551 [!1841](https://gitlab.com/Remmina/Remmina/merge_requests/1841) *@antenore*
* Fixes #1904 - Change Address to URL [!1842](https://gitlab.com/Remmina/Remmina/merge_requests/1842) *@antenore*
* Adding default settings on startup [!1843](https://gitlab.com/Remmina/Remmina/merge_requests/1843) *@antenore*
* Adding default settings on startup [!1844](https://gitlab.com/Remmina/Remmina/merge_requests/1844) *@antenore*
* flatpak: enable smartcard support [!1834](https://gitlab.com/Remmina/Remmina/merge_requests/1834) *@larchunix*
* WWW screenshot features [!1845](https://gitlab.com/Remmina/Remmina/merge_requests/1845) *@antenore*
* WWW Plugin improvements [!1846](https://gitlab.com/Remmina/Remmina/merge_requests/1846) *@antenore*
* Multiple fixes: duplicate tab, remmina_file warning, audible bell setting [!1847](https://gitlab.com/Remmina/Remmina/merge_requests/1847) *@antenore*
* Workaround for #1633 - re-ask user and password when auth error [!1848](https://gitlab.com/Remmina/Remmina/merge_requests/1848) *@antenore*
* Www enhance2 [!1850](https://gitlab.com/Remmina/Remmina/merge_requests/1850) *@antenore*
* Remmina main window improvements to gain space and reduce code [!1851](https://gitlab.com/Remmina/Remmina/merge_requests/1851) *@antenore*
* Updating italian translations [!1852](https://gitlab.com/Remmina/Remmina/merge_requests/1852) *@antenore*
* Updating German translation [!1853](https://gitlab.com/Remmina/Remmina/merge_requests/1853) *@jweberhofer*
* Complete the French translation [!1854](https://gitlab.com/Remmina/Remmina/merge_requests/1854) *@DevDef*
* Fixing typos of X.Org, Java and H.264 [!1855](https://gitlab.com/Remmina/Remmina/merge_requests/1855) *@antenore*
* Translations fixes [!1856](https://gitlab.com/Remmina/Remmina/merge_requests/1856) *@antenore*
* Complete the French translation [!1858](https://gitlab.com/Remmina/Remmina/merge_requests/1858) *@DevDef*
* Updated German translation [!1857](https://gitlab.com/Remmina/Remmina/merge_requests/1857) *@jweberhofer*
* www policies [!1859](https://gitlab.com/Remmina/Remmina/merge_requests/1859) *@antenore*
* Remove msgmerge -N option to enable fuzzy strings [!1860](https://gitlab.com/Remmina/Remmina/merge_requests/1860) *@DevDef*
* One more string translated to French [!1861](https://gitlab.com/Remmina/Remmina/merge_requests/1861) *@DevDef*
* Replace ru.po [!1862](https://gitlab.com/Remmina/Remmina/merge_requests/1862) *@TreefeedXavier*
* Update Croatian language [!1864](https://gitlab.com/Remmina/Remmina/merge_requests/1864) *@muzena*
* Remove outdated strings [!1865](https://gitlab.com/Remmina/Remmina/merge_requests/1865) *@DevDef*
* Fixing preferences menu in the rcw toolbar [!1866](https://gitlab.com/Remmina/Remmina/merge_requests/1866) *@antenore*
* UI Stats and News refactoring [!1868](https://gitlab.com/Remmina/Remmina/merge_requests/1868) *@antenore*
* Update tr.po [!1869](https://gitlab.com/Remmina/Remmina/merge_requests/1869) *@TeknoMobil*
* Update da.po [!1867](https://gitlab.com/Remmina/Remmina/merge_requests/1867) *@scootergrisen*
* Removed unused string in the remmina_news.glade file [!1870](https://gitlab.com/Remmina/Remmina/merge_requests/1870) *@antenore*
* Complete the French translation [!1871](https://gitlab.com/Remmina/Remmina/merge_requests/1871) *@DevDef*
* Adding support for internationalization [!1872](https://gitlab.com/Remmina/Remmina/merge_requests/1872) *@antenore*
* Kwallet [!1873](https://gitlab.com/Remmina/Remmina/merge_requests/1873) *@giox069*
* remmina.pref readonly patches [!1874](https://gitlab.com/Remmina/Remmina/merge_requests/1874) *@antenore*
* Add uid to news GET request [!1876](https://gitlab.com/Remmina/Remmina/merge_requests/1876) *@giox069*
* Remember last quickconnect selected protocol [!1875](https://gitlab.com/Remmina/Remmina/merge_requests/1875) *@giox069*
* Refactoring - warnings cleanup [!1877](https://gitlab.com/Remmina/Remmina/merge_requests/1877) *@antenore*

## v1.3.4
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.3.3...v.1.3.4)

* Updated to use core18 and gnome-3-28-1804 [!1797](https://gitlab.com/Remmina/Remmina/merge_requests/1797) *@kenvandine*
* Snap: Build snap in CI and publish to the edge channel for builds against master [!1810](https://gitlab.com/Remmina/Remmina/merge_requests/1810) *@kenvandine*
* Resolve "SSH public key cannot be imported: Access denied for 'none'. Authentication that can continie: publickey" [!1811](https://gitlab.com/Remmina/Remmina/merge_requests/1811) *@antenore*
* snap: Ensure the icon is installed [!1812](https://gitlab.com/Remmina/Remmina/merge_requests/1812) *@kenvandine*

## v1.3.3
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.3.2...v1.3.3)

* Fix “Utranslated” typo + XHTML 1.0 strictness + move div CSS rule in style block [!1785](https://gitlab.com/Remmina/Remmina/merge_requests/1785) *@DevDef*
* Revert autoclosed <script> tags. It seems to be badly surported [!1786](https://gitlab.com/Remmina/Remmina/merge_requests/1786) *@DevDef*
* Remmina connection window refactoring [!1787](https://gitlab.com/Remmina/Remmina/merge_requests/1787) *@antenore*
* Adding Serial and parallel ports sharing [!1788](https://gitlab.com/Remmina/Remmina/merge_requests/1788) *@antenore*
* Translation of the new strings [!1790](https://gitlab.com/Remmina/Remmina/merge_requests/1790) *@DevDef*
* Update de.po [!1789](https://gitlab.com/Remmina/Remmina/merge_requests/1789) *@OzzieIsaacs*
* Updating zh_CN.po from Zheng Qian [!1792](https://gitlab.com/Remmina/Remmina/merge_requests/1792) *@antenore*
* L10N Updating Italian translations [!1793](https://gitlab.com/Remmina/Remmina/merge_requests/1793) *@antenore*
* RemminaMain window refactoring - Removing deprecated functions. [!1794](https://gitlab.com/Remmina/Remmina/merge_requests/1794) *@antenore*
* Update fr.po [!1795](https://gitlab.com/Remmina/Remmina/merge_requests/1795) *@DevDef*
* Fix #1836 implementing the correct message panel when authenticating [!1796](https://gitlab.com/Remmina/Remmina/merge_requests/1796) *@antenore*
* Make ssh tunnel pwd user manageable and public key import [!1798](https://gitlab.com/Remmina/Remmina/merge_requests/1798) *@giox069*
* Update tr.po turkish translation [!1799](https://gitlab.com/Remmina/Remmina/merge_requests/1799) *@TeknoMobil*
* Translate 3 new strings [!1800](https://gitlab.com/Remmina/Remmina/merge_requests/1800) *@DevDef*
* Vnci fixes [!1801](https://gitlab.com/Remmina/Remmina/merge_requests/1801) *@antenore*
* Update fr.po [!1802](https://gitlab.com/Remmina/Remmina/merge_requests/1802) *@DevDef*
* Translate new strings to German [!1803](https://gitlab.com/Remmina/Remmina/merge_requests/1803) *@jweberhofer*
* Fix Yes/No inversion [!1805](https://gitlab.com/Remmina/Remmina/merge_requests/1805) *@DevDef*
* Update Danish translation [!1804](https://gitlab.com/Remmina/Remmina/merge_requests/1804) *@scootergrisen*
* Remmina release v.1.3.3 [!1807](https://gitlab.com/Remmina/Remmina/merge_requests/1807) *@antenore*

## [v1.3.2](https://gitlab.com/Remmina/Remmina/tags/v1.3.2) (2019-01-31)

* Change rcw size allocation wait algorithm, see issue #1809 (by Giovanni Panozzo)
* Fix a couple of VNCI crashes, see issue #1821 (by Giovanni Panozzo)
* Fix spice authentication, issue #1820 (by Giovanni Panozzo)
* Update translations script fixes (by Antenore Gatta)
* Add a missing end point in an SSH error message (by Davy Defaud)
* Complete the French translation (by Davy Defaud)
* New strings for it.po (by Giovanni Panozzo)
* Remmina Translations Statistics (by Antenore Gatta)
* Remmina Translation Report (by Antenore Gatta)
* Cosmetics fixes: (by Antenore Gatta & Davy Defaud)
* Development Documentation fixes (by Antenore Gatta)

## [v1.3.1](https://gitlab.com/Remmina/Remmina/tags/v1.3.1) (2019-01-28)

### Bug Fixing release

[Giovanni Panozzo](@giox069) has fixed many issues that have been introduced during the last release.

Additionally a great rework has been done by [Davy Defaud](@DevDef) to fix many
typographic errors and translations.

[Wolfgang Scherer](@wolfmanx) has fixed an annoying clipboard bug in the VNC plugin.

### List of changes

* Updated author files (Antenore Gatta)
* Version bump (Antenore Gatta)
* Merge branch 'german-translation' into 'master' (Davy Defaud)
* Updated german translation (Johannes Weberhofer)
* Updated translations (Antenore Gatta)
* Fixed pot generation (Antenore Gatta)
* Fixed LINGUAS format (Antenore Gatta)
* Merge branch 'typographic_fixes' into 'master' (Antenore Gatta)
* Use uppercase for VNC, SSH, SFTP and RDP protocol names (Davy Defaud)
* Gettextize more SSH error messages (Davy Defaud)
* Replace single quotes by true apostrophes (Davy Defaud)
* Replace all triple points by ellipsis characters (Davy Defaud)
* Remove space before colons in English strings (Davy Defaud)
* Fix English typo transmittion → transmission (Davy Defaud)
* Fix a bad indent (spaces instead of a tab) (Davy Defaud)
* Merge branch 'vnc-blocks-after-clipboard' into 'master' (Giovanni Panozzo)
* VNC: prevent client from blocking when remote clipboard is changed (Wolfgang Scherer)
* Merge branch 'master' into 'master' (Davy Defaud)
* Updated ru.po for more human-like language (TreefeedXavier)
* Merge branch 'patch-5' into 'master' (Davy Defaud)
* Performance improvement (Antenore Gatta)
* Update tr.po (Serdar Sağlam)
* Bump documentation version (Antenore Gatta)
* Update tr.po (Serdar Sağlam)
* Fixing docker image (Antenore Gatta)
* Update tr.po (Serdar Sağlam)
* Update tr.po (Serdar Sağlam)
* Update tr.po (Serdar Sağlam)
* Update tr.po (Serdar Sağlam)
* Add background class to message panel (Giovanni Panozzo)
* Merge branch 'css_fix' into 'master' (Giovanni Panozzo)
* Fix password field focusing, see issue #1811 (Giovanni Panozzo)
* Fix CSS of message panel and tab page (Giovanni Panozzo)
* Merge branch 'master' of gitlab.com:Remmina/Remmina (Antenore Gatta)
* Logo documentation (Antenore Gatta)
* Merge branch 'italian-strings' into 'master' (Davy Defaud)
* Remove the duplicate of the first header line (Davy Defaud)
* Some strings for it.po (Giovanni Panozzo)
* Merge branch 'rcw_fixess' into 'master' (Giovanni Panozzo)
* rcw fixes for fullscreen mode (Giovanni Panozzo)
* all rows translated (Serdar Sağlam)
* Merge branch 'master' of https://gitlab.com/Remmina/Remmina (Giovanni Panozzo)
* Fix border appearing in VIEWPORT_FULLSCREEN_MODE when not needed (Giovanni Panozzo)
* Merge branch '1813-a-few-fixes-improvements-for-the-po-files' into 'master' (Davy Defaud)
* Fix scrolling in VIEWPORT_FULLSCREEN_MODE (Giovanni Panozzo)
* Correctly reparent MessagePanel when moving a tab (Giovanni Panozzo)
* Make ftb visible before connecting (Giovanni Panozzo)
* Add the missing plural forms of a translation (yet to be translated…) (Davy Defaud)
* Add missing format specification in a Spanish translation string (Davy Defaud)
* Fix missing language code in po some file headers (Davy Defaud)
* Merge branch 'patch-4' into 'master' (Antenore Gatta)
* Update tr.po (Serdar Sağlam)
* Merge branch 'blackmp' into 'master' (Antenore Gatta)
* remove black background (Antenore Gatta)
* Merge branch 'issue-1809' into 'master' (Giovanni Panozzo)
* Set notebook current page to 1st page on new cnnwin (Giovanni Panozzo)
* Merge branch 'issue1810-icons' into 'master' (Antenore Gatta)
* Removed edit-delete.svg from the repository (Antenore Gatta)
* Merge branch 'issue1810-icons' into 'master' (Antenore Gatta)
* Removed edit-delete icon - fixes #1810 (Antenore Gatta)
* Merge branch 'patch-1' into 'master' (Antenore Gatta)
* flatpak: update spice-gtk from 0.35 to 0.36 (Denis Ollier)
* Translate the two new strings recently added (Davy Defaud)

## [v1.3.0](https://gitlab.com/Remmina/Remmina/tags/v1.3.0) (2019-01-18)

### Most notable

* Use window resolution #1 (by Giovanni Panozzo)
* rcw_preopen complete (by Giovanni Panozzo)
* RDP: new global parameter rdp_map_keycode (by Giovanni Panozzo)
* Use decimal instead of hex on rdp keycode map (by Giovanni Panozzo)

### Translations

* Adding basic script to update the po files (by Antenore Gatta)
* Added remmina.pot (by Antenore Gatta)
* Change es_VE.po header (Language-Team and Language) (by Antenore Gatta)
* Fixed some Italian translations (by Antenore Gatta)
* Fixing es_VE Spanish #1797 (by Antenore Gatta)
* French translation update for v1.3.0 (by Davy Defaud)
* Improving i18n subsystem (by Antenore Gatta)
* Russian translation updated (fully translated) (by TreefeedXavier)
* Translations - Fixing errors reported in the issue #1797 (by Antenore Gatta)
* Update Hungarian translation (by Meskó Balázs)
* Update de.po (by Ozzie Isaacs)
* Update tr.po (by Serdar Sağlam)
* Updated german translation (by Ozzie Isaacs)
* Updated ru.po (by TreefeedXavier)
* Updated translators list ru.po (by TreefeedXavier)
* Updating da.po (by Antenore Gatta)
* Updating po translation files (by Antenore Gatta)
* Updating translation credits (by Antenore Gatta)

### Other enhancements and fixes

* Added xrdp friendly options Closes #1742 (by Antenore Gatta)
* Adding language detection (by Antenore Gatta)
* Auth panel widget placement (by Antenore Gatta)
* CSS modifications to adapt to stock Gnome and Gtk themes (by Antenore Gatta)
* Correctly set focus after rcw_preopen (by Giovanni Panozzo)
* Deprecates dynamic_resolution_width and height cfg params (by Giovanni Panozzo)
* Disable glyph cache by default (by Antenore Gatta)
* Fix crash when showing password panel (by Giovanni Panozzo)
* Fix gtk_window_present_with_time() time (by Giovanni Panozzo)
* Fixed build for the flatpak (by Antenore Gatta)
* Fixed missing icons (by Antenore Gatta)
* Gtk deprecation and CSS restzling (by Antenore Gatta)
* Gtk icon cache update during install phase (by Antenore Gatta)
* Icons and gtk fixes for rcw_reopen (by Antenore Gatta)
* Make menu items paintable by the application (by Antenore Gatta)
* Open connection window before connecting (by Giovanni Panozzo)
* Prevent toolbar signals while reconfiguring toolbar (by Giovanni Panozzo)
* RDP fixes: remove redundant rfi->width/rfi->height and more (by Giovanni Panozzo)
* RDP: correctly destroy rfi->surface during a desktop resize (by Giovanni Panozzo)
* RDP: move gdi_resize() to a better place (by Giovanni Panozzo)
* RDP: remove unneeded OrderSupport struct init (by Giovanni Panozzo)
* Remove deprecated floating toolbar toplevel window (by Giovanni Panozzo)
* Search box clear icon (by Giovanni Panozzo)
* Update CONTRIBUTING.md (by Antenore Gatta)
* Update README.md (by Antenore Gatta)
* Update toolbar button handling (by Giovanni Panozzo)
* Updated CSS to have black background in fullscreen (by Antenore Gatta)
* Updated sponsor list (by Antenore Gatta)
* Updated wiki URLs (by Antenore Gatta)
* Updating Doxygen config (by Antenore Gatta)
* Updating coyright for year 2019 (by Antenore Gatta)
* VNC: Fix possible crash during connection (by Giovanni Panozzo)
* allow closing tab after error message panel is shown (by Giovanni Panozzo)
* flatpak: update freerdp from 2.0.0-rc3 to 2.0.0-rc4 (by Denis Ollier)
* flatpak: update gnome sdk from 3.28 to 3.30 (by Denis Ollier)
* flatpak: update libssh from 0.8.5 to 0.8.6 (by Denis Ollier)
* flatpak: update libvncserver from 0.9.11 to 0.9.12 (by Denis Ollier)
* flatpak: update nx-libs from 3.5.99.16 to 3.5.99.17 (by Denis Ollier)
* flatpak: update six from 1.11.0 to 1.12.0 (by Denis Ollier)

## [v1.2.32.1](https://gitlab.com/Remmina/Remmina/tags/v1.2.32.1) (2018-11-14)

* Add desktop-gnome-platform and fix themes in SNAP, fixes issue #1730 and fixes missing SNAP localization (Giovanni Panozzo)
* Fix SNAP icon (Giovanni Panozzo)
* flatpak: update libssh from 0.8.2 to 0.8.3 (Denis Ollier)
* flatpak: update libssh from 0.8.3 to 0.8.4 (Denis Ollier)
* flatpak: update libssh from 0.8.4 to 0.8.5 (Denis Ollier)
* flatpak: update lz4 from 1.8.2 to 1.8.3 (Denis Ollier)
* flatpak: update pyparsing from 2.2.0 to 2.2.1 (Denis Ollier)
* flatpak: update pyparsing from 2.2.1 to 2.2.2 (Denis Ollier)
* flatpak: update pyparsing from 2.2.2 to 2.3.0 (Denis Ollier)
* Implement smartcard name setting. Should fix #1737 (Antenore Gatta)
* man+help: elaborate on file types of -connect and -edit cmd line options (Mikkel Kirkgaard Nielsen)
* RDP: add FREERDP_ERROR_SERVER_DENIED_CONNECTION message (Giovanni Panozzo)
* Removing X11Forwarding code as it is wrong and causing issues (Antenore Gatta)
* Update fix tr.po (Serdar Sağlam)

## [v1.2.32](https://gitlab.com/Remmina/Remmina/tags/v1.2.32) (2018-10-06)

* Avoid to save last_success property if stats are not enabled. @antenore
* GW auth data was saved in the server auth data. @antenore
* Do not register socket plugins when X11 is not available. @antenore
* Screenshot enhancements. @antenore
* RDP GW authentication. @antenore
* Adding global preference for search bar visibility. @antenore
* Allow wayland backend again when GTK >= 3.22.27. @giox069
* tr.po @TeknoMobil
* Add option to honour https_proxy and http_proxy environment variable. @antenore
* Force program name to app id. @antenore
* Printing builds flags with remmina --full-version command option. @antenore
* New plugin Simple Terminal. @antenore
* Fix KB grabbing when switching workspace. @giox069
* Dealing correcthly with some deprecations, getting rid of most of G_GNUC_BEGIN_IGNORE_DEPRECATIONS. @antenore
* Improving file type hadling. @antenore
* Adding error check on remmina_pref_save. @antenore
* Many bug fixing as usual.

## [v1.2.31.4](https://gitlab.com/Remmina/Remmina/tags/v1.2.31.4) (2018-08-24)

This is a bug fixing release.

**Fixed bugs:**

* Fix KB grabbing when switching workspace.
* Fix some possible crashes when reading a remminafile.
* Fixes a crash deleting XDMCP profile.
* Fixing libssh deprecations.

**Implemented enhancements:**

* Improving file type hadling.
* flatpak: update libssh from 0.7.5 to 0.8.1
* flatpak: update freerdp from 2.0.0-rc2 to 2.0.0-rc3
* Snap: update to libssh 0.8.0.

## [v1.2.31.3](https://gitlab.com/Remmina/Remmina/tags/v1.2.31.3) (2018-08-14)

This is a bug fixing release.

**Fixed bugs:**

* Do not send stats if the remmina.pref file is read-only.
* cmake: include libssh_threads only when available.
* Set program class to REMMINA_APP_ID, fixes #1706.

**Implemented enhancements:**

* Implement send ctrl+alt+fn keys. Closes #1707.

## [v1.2.31.1](https://gitlab.com/Remmina/Remmina/tags/v1.2.31.1) (2018-07-31)

This is a bug fixing release.

**Fixed bugs:**

* Cannot minimize in fullscreen mode.
* Crash with the RDP plugin.
* Missing manual pages for Debian.

**Closed issues:**

* Minimize window doesn't minimize the window [\#1696](https://gitlab.com/Remmina/Remmina/issues/1696)
* Minimize window button is disable, after connecting to RDS [\#1700](https://gitlab.com/Remmina/Remmina/issues/1700)

## [v1.2.31](https://gitlab.com/Remmina/Remmina/tags/v1.2.31) (2018-07-28)

This is the first release on GitLab and we are still moving and adapting tools and
integrations, therefore this changelog is quite short than usually.

Notables changes since the last release are:

* Custom color schemes per profile for the SSH plugin (@denk_mal).
* Flatpak updates and fixes (@larchunix)
* Kiosk mode with integration in the login manager (@antenore).
* New Icons (@antenore and @larchunix).
* SFTP tool password fixes (@Feishi).
* Several fixes around RDP and compilations issues (@giox069 and @larchunix).

