CREATE TABLE file(
	id          INTEGER PRIMARY KEY AUTOINCREMENT,
	path        TEXT    NOT NULL UNIQUE,
	name        TEXT    NOT NULL,
	dir         TEXT    NOT NULL,
	alias       TEXT    NOT NULL,
	state       INTEGER NOT NULL,
	comment     TEXT    NOT NULL,
	source      TEXT    NOT NULL,
	sha1        BLOB    NOT NULL,
	created     INTEGER NOT NULL DEFAULT (unixepoch()),
	modified    INTEGER NOT NULL DEFAULT (unixepoch())
) STRICT;

CREATE INDEX file_path ON file(path);
CREATE INDEX file_name ON file(name);
CREATE INDEX file_dir  ON file(dir);

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

CREATE TABLE file_tag(
	file_id INTEGER NOT NULL,
	tag_id  INTEGER NOT NULL,
	created INTEGER NOT NULL DEFAULT (unixepoch()),
	PRIMARY KEY (file_id, tag_id),
	FOREIGN KEY (file_id) REFERENCES file(id) ON DELETE CASCADE,
	FOREIGN KEY (tag_id)  REFERENCES tag(id)  ON DELETE CASCADE
) STRICT;

CREATE TRIGGER increment_tag_degree
AFTER INSERT ON file_tag
BEGIN
	UPDATE tag
	SET    degree = degree + 1
	WHERE  id = NEW.tag_id;
END;

CREATE TRIGGER decrement_tag_degree
AFTER DELETE ON file_tag
BEGIN
	UPDATE tag
	SET    degree = degree - 1
	WHERE  id = OLD.tag_id;
END;

CREATE TRIGGER update_file_modified
AFTER UPDATE OF path, dir, alias, comment, source, sha1digest ON file
BEGIN
	UPDATE file
	SET    modified = unixepoch()
	WHERE  id = file.id;
END;

CREATE TRIGGER update_tag_modified
AFTER UPDATE OF name, description, urls ON tag
BEGIN
	UPDATE tag
	SET    modified = unixepoch()
	WHERE  id = tag.id;
END;
