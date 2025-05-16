#
#TODO extract version from src/compile_constants.h
touch changelog
printf "sdl-img (0.101.0) unstable; urgency=medium\n\n" > changelog
echo "`git log $(git describe --tags --abbrev=0)..HEAD --pretty=format:'  * %h %s'`" >> changelog
printf " -- Robert Winkler <rob121618@gmail.com>  " >> changelog
echo `date -R` >> changelog
