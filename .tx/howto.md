Go to the [Transifex client documentation](http://help.transifex.com/features/client/index.html) for more info.

## Initializing a new project on Transifex

<pre>
tx init
tx set --auto-local -r hexchat.main "po\<lang>.po" --source-lang en --source-file po\hexchat.pot --execute
</pre>

Append `type = PO` to _.tx\config_.

Push the resources to Transifex:

<pre>
tx push --source --translation
</pre>


## Updating online translations with the template

Regenerate the source file ( _hexchat.pot_ ) on a Unix machine:

<pre>
rm po/hexchat.pot && ./autogen.sh && ./configure --enable-nls && cd src/common && make textevents.h && cd../.. && make
</pre>

Push the updated source file to Transifex (this automatically updates all translation files):

<pre>
tx push --source
</pre>


## Updating the repo with online translations

Update local copy with Transifex updates:

<pre>
tx pull
</pre>

Update GitHub repo:

<pre>git add po/
git commit
git push
</pre>


## Updating just one translation with local changes

<pre>
tx push --translation --language xy
</pre>

Where _xy_ is the language code.


## Forcing translation updates

<pre>
tx pull --force
</pre>

This might be required when the repo is freshly cloned and thus timestamps are newer than on Transifex.
