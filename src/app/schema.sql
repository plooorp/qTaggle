CREATE TABLE file(
	id          INTEGER PRIMARY KEY AUTOINCREMENT,
	path        TEXT    NOT NULL UNIQUE,
	name        TEXT    NOT NULL,
	alias       TEXT    NOT NULL,
	state       INTEGER NOT NULL,
	comment     TEXT    NOT NULL,
	source      TEXT    NOT NULL,
	sha1        BLOB    NOT NULL,
	created     INTEGER NOT NULL DEFAULT (unixepoch()),
	modified    INTEGER NOT NULL DEFAULT (unixepoch()),
	checked     INTEGER NOT NULL DEFAULT (unixepoch())
) STRICT;

CREATE INDEX file_path ON file(path);
CREATE INDEX file_name ON file(name);
CREATE INDEX file_alias ON file(alias);
CREATE INDEX file_state ON file(state);
CREATE INDEX file_created ON file(created);
CREATE INDEX file_modified ON file(modified);
CREATE INDEX file_checked ON file(checked);

CREATE VIRTUAL TABLE file_search USING fts5(name, alias, path, comment, content='file', content_rowid='id');
CREATE TRIGGER file_ai AFTER INSERT ON file
BEGIN
	INSERT INTO file_search(rowid, name, alias, path, comment)
	VALUES (NEW.id, NEW.name, NEW.alias, NEW.path, NEW.comment);
END;
CREATE TRIGGER file_ad AFTER DELETE ON file
BEGIN
	INSERT INTO file_search(file_search, rowid, name, alias, path, comment)
	VALUES ('delete', OLD.id, OLD.name, OLD.alias, OLD.path, OLD.comment);
END;
CREATE TRIGGER file_au AFTER UPDATE ON file
BEGIN
	INSERT INTO file_search(file_search, rowid, name, alias, path, comment)
	VALUES ('delete', OLD.id, OLD.name, OLD.alias, OLD.path, OLD.comment);
	INSERT INTO file_search(rowid, name, alias, path, comment)
	VALUES (NEW.id, NEW.name, NEW.alias, NEW.path, NEW.comment);
END;

CREATE TABLE tag(
	id          INTEGER PRIMARY KEY AUTOINCREMENT,
	name        TEXT    NOT NULL UNIQUE,
	description TEXT    NOT NULL,
	urls        TEXT    NOT NULL,
	degree      INTEGER NOT NULL DEFAULT 0,
	created     INTEGER NOT NULL DEFAULT (unixepoch()),
	modified    INTEGER NOT NULL DEFAULT (unixepoch())
) STRICT;

CREATE INDEX tag_name ON tag(name);
CREATE INDEX tag_degree ON tag(degree);
CREATE INDEX tag_created ON tag(created);
CREATE INDEX tag_modified ON tag(modified);

CREATE VIRTUAL TABLE tag_search USING fts5(name, description, content='tag', content_rowid='id');
CREATE TRIGGER tag_ai AFTER INSERT ON tag
BEGIN
	INSERT INTO tag_search(rowid, name, description)
	VALUES (NEW.id, NEW.name, NEW.description);
END;
CREATE TRIGGER tag_ad AFTER DELETE ON tag
BEGIN
	INSERT INTO tag_search(tag_search, rowid, name, description)
	VALUES ('delete', OLD.id, old.name, old.description);
END;
CREATE TRIGGER tag_au AFTER UPDATE ON tag
BEGIN
	INSERT INTO tag_search(tag_search, rowid, name, description)
	VALUES ('delete', OLD.id, OLD.name, OLD.description);
	INSERT INTO tag_search(rowid, name, description)
	VALUES (NEW.id, NEW.name, NEW.description);
END;

CREATE TABLE file_tag(
	file_id INTEGER NOT NULL,
	tag_id  INTEGER NOT NULL,
	created INTEGER NOT NULL DEFAULT (unixepoch()),
	PRIMARY KEY (file_id, tag_id),
	FOREIGN KEY (file_id) REFERENCES file(id) ON DELETE CASCADE,
	FOREIGN KEY (tag_id)  REFERENCES tag(id)  ON DELETE CASCADE
) STRICT;

CREATE TRIGGER update_file_modified
AFTER UPDATE OF alias, comment, source, sha1digest ON file
BEGIN
	UPDATE file
	SET    modified = unixepoch()
	WHERE  id = NEW.id;
END;

CREATE TRIGGER update_tag_modified
AFTER UPDATE OF name, description, urls ON tag
BEGIN
	UPDATE tag
	SET    modified = unixepoch()
	WHERE  id = NEW.id;
END;

CREATE TRIGGER file_tag_ai AFTER INSERT ON file_tag
BEGIN
	UPDATE file SET modified = unixepoch() WHERE file.id = NEW.file_id;
	UPDATE tag SET degree = degree + 1 WHERE id = NEW.tag_id;
END;

CREATE TRIGGER file_tag_ad AFTER DELETE ON file_tag
BEGIN
	UPDATE file SET modified = unixepoch() WHERE file.id = OLD.file_id;
	UPDATE tag SET degree = degree - 1 WHERE id = OLD.tag_id;
END;
